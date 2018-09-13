/*
 *  DISP.CXX
 *
 *  Purpose:
 *      CDisplay class
 *
 *  Owner:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 */

#include "headers.hxx"

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_MISCPROT_H_
#define X_MISCPROT_H_
#include "miscprot.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_DOCPRINT_HXX_
#define X_DOCPRINT_HXX_
#include "docprint.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_RCLCLPTR_HXX_
#define X_RCLCLPTR_HXX_
#include "rclclptr.hxx"
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include "table.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifndef X_DISPITEMPLUS_HXX
#define X_DISPITEMPLUS_HXX
#include "dispitemplus.hxx"
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

EXTERN_C const GUID CLSID_HTMLCheckboxElement;
EXTERN_C const GUID CLSID_HTMLRadioElement;

// Timer tick counts for background task
#define cmsecBgndInterval   300
#define cmsecBgndBusy       100

// Lines ahead
const LONG g_cExtraBeforeLazy = 10;

PerfDbgTag(tagRecalc, "Layout Recalc Engine", "Layout recalcEngine");
DeclareTag(tagPositionObjects, "PositionObjects", "PositionObjects");
DeclareTag(tagRenderingRect, "Rendering rect", "Rendering rect");
DeclareTag(tagRelDispNodeCache, "Relative disp node cache", "Trace changes to disp node cache");
DeclareTag(tagTableCalcDontReuseMeasurer, "Tables", "Disable measurer reuse across Table cells");
PerfDbgExtern(tagPaintWait);

MtDefine(CRecalcTask, Layout, "CRecalcTask")
MtDefine(CDisplay, Layout, "CDisplay")
MtDefine(CDisplay_aryRegionCollection_pv, CDisplay, "CDisplay::_aryRegionCollection::_pv")
MtDefine(CRelDispNodeCache, CDisplay, "CRelDispNodeCache::_pv")
MtDefine(CDisplayUpdateView_aryInvalRects_pv, Locals, "CDisplay::UpdateView aryInvalRects::_pv")
MtDefine(CDisplayDrawBackgroundAndBorder_aryRects_pv, Locals, "CDisplay::DrawBackgroundAndBorder aryRects::_pv")
MtDefine(CDisplayDrawBackgroundAndBorder_aryNodesWithBgOrBorder_pv, Locals, "CDisplay::DrawBackgroundAndBorder aryNodesWithBgOrBorder::_pv")

//
// This function does exactly what IntersectRect does, except that
// if one of the rects is empty, it still returns TRUE if the rect
// is located inside the other rect. [ IntersectRect rect in such
// case returns FALSE. ]
//

BOOL
IntersectRectE (RECT * prRes, const RECT * pr1, const RECT * pr2)
{
    // nAdjust is used to control what statement do we use in conditional
    // expressions: a < b or a <= b. nAdjust can be 0 or 1;
    // when (nAdjust == 0): (a - nAdjust < b) <==> (a <  b)  (*)
    // when (nAdjust == 1): (a - nAdjust < b) <==> (a <= b)  (**)
    // When at least one of rects to intersect is empty, and the empty
    // rect lies on boundary of the other, then we consider that the
    // rects DO intersect - in this case nAdjust == 0 and we use (*).
    // If both rects are not empty, and rects touch, then we should
    // consider that they DO NOT intersect and in that case nAdjust is
    // 1 and we use (**).
    //
    int nAdjust;

    Assert (prRes && pr1 && pr2);
    Assert (pr1->left <= pr1->right && pr1->top <= pr1->bottom &&
            pr2->left <= pr2->right && pr2->top <= pr2->bottom);

    prRes->left  = max (pr1->left,  pr2->left);
    prRes->right = min (pr1->right, pr2->right);
    nAdjust = (int) ( (pr1->left != pr1->right) && (pr2->left != pr2->right) );
    if (prRes->right - nAdjust < prRes->left)
        goto NoIntersect;

    prRes->top    = max (pr1->top,  pr2->top);
    prRes->bottom = min (pr1->bottom, pr2->bottom);
    nAdjust = (int) ( (pr1->top != pr1->bottom) && (pr2->top != pr2->bottom) );
    if (prRes->bottom - nAdjust < prRes->top)
        goto NoIntersect;

    return TRUE;

NoIntersect:
    SetRect (prRes, 0,0,0,0);
    return FALSE;
}

#if DBG == 1
//
// because IntersectRectE is quite fragile on boundary cases and these
// cases are not obvious, and also because bugs on these boundary cases
// would manifest in a way difficult to debug, we use this function to
// assert (in debug build only) that the function returns results we
// expect.
//
void
AssertIntersectRectE ()
{
    struct  ASSERTSTRUCT
    {
        RECT    r1;
        RECT    r2;
        RECT    rResExpected;
        BOOL    fResExpected;
    };

    ASSERTSTRUCT ts [] =
    {
        //  r1                  r2                  rResExpected      fResExpected
        // 1st non-empty, no intersect
        { {  0,  2, 99,  8 }, {  0, 10, 99, 20 }, {  0,  0,  0,  0 }, FALSE },
        { {  0, 22, 99, 28 }, {  0, 10, 99, 20 }, {  0,  0,  0,  0 }, FALSE },
        // 1st non-empty, intersect
        { {  0,  2, 99, 18 }, {  0, 10, 99, 20 }, {  0, 10, 99, 18 }, TRUE  },
        { {  0, 12, 99, 28 }, {  0, 10, 99, 20 }, {  0, 12, 99, 20 }, TRUE  },
        { {  0, 12, 99, 18 }, {  0, 10, 99, 20 }, {  0, 12, 99, 18 }, TRUE  },
        { {  0,  2, 99, 28 }, {  0, 10, 99, 20 }, {  0, 10, 99, 20 }, TRUE  },
        // 1st non-empty, touch
        { {  0,  2, 99, 10 }, {  0, 10, 99, 20 }, {  0,  0,  0,  0 }, FALSE },
        { {  0, 20, 99, 28 }, {  0, 10, 99, 20 }, {  0,  0,  0,  0 }, FALSE },

        // 1st empty, no intersect
        { {  0,  2, 99,  2 }, {  0, 10, 99, 20 }, {  0,  0,  0,  0 }, FALSE },
        { {  0, 28, 99, 28 }, {  0, 10, 99, 20 }, {  0,  0,  0,  0 }, FALSE },
        // 1st empty, intersect
        { {  0, 12, 99, 12 }, {  0, 10, 99, 20 }, {  0, 12, 99, 12 }, TRUE  },
        // 1st empty, touch
        { {  0, 10, 99, 10 }, {  0, 10, 99, 20 }, {  0, 10, 99, 10 }, TRUE  },
        { {  0, 20, 99, 20 }, {  0, 10, 99, 20 }, {  0, 20, 99, 20 }, TRUE  },

        // both empty
        { {  0, 10, 99, 10 }, {  0, 10, 99, 10 }, {  0, 10, 99, 10 }, TRUE  }
    };

    ASSERTSTRUCT *  pts;
    RECT            r1;
    RECT            r2;
    RECT            rResActual;
    RECT            rResExpected;
    BOOL            fResActual;
    int             c;

    for (
        c = ARRAY_SIZE(ts), pts = &ts[0];
        c;
        c--, pts++)
    {
        // test
        fResActual = IntersectRectE(&rResActual, &pts->r1, &pts->r2);
        if (!EqualRect(&rResActual, &pts->rResExpected) || fResActual != pts->fResExpected)
            goto Failed;

        // now swap rects and test
        fResActual = IntersectRectE(&rResActual, &pts->r2, &pts->r1);
        if (!EqualRect(&rResActual, &pts->rResExpected) || fResActual != pts->fResExpected)
            goto Failed;

        // now swap left<->top and right<->bottom
        //   swapped left         top           right           bottom
        SetRect (&r1, pts->r1.top, pts->r1.left, pts->r1.bottom, pts->r1.right);
        SetRect (&r2, pts->r2.top, pts->r2.left, pts->r2.bottom, pts->r2.right);
        SetRect (&rResExpected, pts->rResExpected.top, pts->rResExpected.left, pts->rResExpected.bottom, pts->rResExpected.right);

        // test
        fResActual = IntersectRectE(&rResActual, &r1, &r2);
        if (!EqualRect(&rResActual, &rResExpected) || fResActual != pts->fResExpected)
            goto Failed;

        // now swap rects and test
        fResActual = IntersectRectE(&rResActual, &r2, &r1);
        if (!EqualRect(&rResActual, &rResExpected) || fResActual != pts->fResExpected)
            goto Failed;

    }

    return;

Failed:
    Assert (0 && "IntersectRectE returns an unexpected result");
}
#endif

// ===========================  CLed  =====================================================


void CLed::SetNoMatch()
{
    _cpMatchNew  = _cpMatchOld  =
    _iliMatchNew = _iliMatchOld =
    _yMatchNew   = _yMatchOld   = MAXLONG;
}


//-------------------- Start: Code to implement background recalc in lightwt tasks

class CRecalcTask : public CTask
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRecalcTask))

    CRecalcTask (CDisplay *pdp, DWORD grfLayout)
    {
        _pdp = pdp ;
        _grfLayout = grfLayout;
    }

    virtual void OnRun (DWORD dwTimeOut)
    {
        _pdp->StepBackgroundRecalc (dwTimeOut, _grfLayout) ;
    }

    virtual void OnTerminate () {}

private:
    CDisplay *_pdp ;
    DWORD     _grfLayout;
} ;

//-------------------- End: Code to implement background recalc in lightwt tasks


// ===========================  CDisplay  =====================================================

CDisplay::~CDisplay()
{
    // The recalc task should have disappeared during the detach!
    Assert (!HasBgRecalcInfo() && !RecalcTask());
}

CElement *
CDisplay::GetFlowLayoutElement() const
{
    return GetFlowLayout()->ElementContent();
}

CMarkup * CDisplay::GetMarkup() const
{
    return GetFlowLayout()->GetContentMarkup();
}

CDisplay::CDisplay ()
{
#if DBG == 1
    _pFL = CONTAINING_RECORD(this, CFlowLayout, _dp);
#endif
    _fRecalcDone = TRUE;

#if DBG == 1
    AssertIntersectRectE ();
#endif
}

//+----------------------------------------------------------------------------
//  Member:     CDisplay::Init
//
//  Synopsis:   Initializes CDisplay
//
//  Returns:    TRUE - initialization succeeded
//              FALSE - initalization failed
//
//+----------------------------------------------------------------------------
BOOL CDisplay::Init()
{
    CFlowLayout * pFL = GetFlowLayout();

    Assert( _yCalcMax     == 0 );        // Verify allocation zeroed memory out
    Assert( _xWidth       == 0 );
    Assert( _yHeight      == 0 );
    Assert( RecalcTask()  == NULL );

    SetWordWrap(pFL->GetWordWrap());

    _xWidthView = 0;

    return TRUE;
}

//+------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Do stuff before dying
//
//-------------------------------------------------------------------------
void
CDisplay::Detach()
{
    // If there's a timer on, get rid of it before we detach the
    // object. This prevents us from trying to recalc the lines
    // after the CTxtSite has gone away.
    FlushRecalc();
}

/*
 *  CDisplay::GetFirstCp
 *
 *  @mfunc
 *      Return the first cp
 */
LONG
CDisplay::GetFirstCp() const
{
    return GetFlowLayout()->GetContentFirstCp();
}

/*
 *  CDisplay::GetLastCp
 *
 *  @mfunc
 *      Return the last cp
 */
LONG
CDisplay::GetLastCp() const
{
    return GetFlowLayout()->GetContentLastCp();
}

/*
 *  CDisplay::GetMaxCpCalced
 *
 *  @mfunc
 *      Return the last cp calc'ed. Note that this is
 *      relative to the start of the site, not the story.
 */
LONG
CDisplay::GetMaxCpCalced() const
{
    return GetFlowLayout()->GetContentFirstCp() + _dcpCalcMax;
}


inline BOOL
CDisplay::AllowBackgroundRecalc(CCalcInfo * pci, BOOL fBackground)
{
    CFlowLayout * pFL = GetFlowLayout();

#ifdef SWITCHES_ENABLED
    if (IsSwitchNoBgRecalc())
        return(FALSE);
#endif

    // Allow background recalc when:
    //  a) Not currently calcing in the background
    //  b) It is a SIZEMODE_NATURAL request
    //  c) The CTxtSite does not size to its contents
    //  d) The site is not part of a print document
    //  e) The site allows background recalc
    return (!fBackground                             &&
            (pci->_smMode == SIZEMODE_NATURAL)       &&
            !(pci->_grfLayout & LAYOUT_NOBACKGROUND) &&
            !pFL->_fContentsAffectSize   &&
            !pFL->GetAutoSize()          &&
            !_fPrinting                  &&
            !pFL->TestClassFlag(CElement::ELEMENTDESC_NOBKGRDRECALC));
}


/*
 *  CDisplay::FlushRecalc()
 *
 *  @mfunc
 *      Destroys the line array, therefore ensure a full recalc next time
 *      RecalcView or UpdateView is called.
 *
 */
void CDisplay::FlushRecalc()
{
    CFlowLayout * pFL = GetFlowLayout();

    StopBackgroundRecalc();

    if (LineCount())
    {
        Remove(0, -1, AF_KEEPMEM);          // Remove all old lines from *this
    }

    TraceTag((tagRelDispNodeCache, "SN: (%d) FlushRecalc:: invalidating rel line cache",
                                GetFlowLayout()->SN()));

    pFL->_fContainsRelative   = FALSE;
    pFL->_fChildWidthPercent  = FALSE;
    pFL->_fChildHeightPercent = FALSE;
    pFL->CancelChanges();

    VoidRelDispNodeCache();
    DestroyFlowDispNodes();

    _fRecalcDone = FALSE;
    _yCalcMax   = 0;                        // Set both maxes to start of text
    _dcpCalcMax = 0;                        // Relative to the start cp of site.
    _xWidth     = 0;
    _yHeight    = 0;
    _yHeightMax = 0;

    _fContainsHorzPercentAttr =
    _fContainsVertPercentAttr =
    _fNavHackPossible      =
    _fHasEmbeddedLayouts   = 
    _fHasMultipleTextNodes = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     NoteMost
//
//  Purpose:    Notes if the line has anything which will need us to compute
//              information for negative lines/absolute or relative divs
//
//----------------------------------------------------------------------------
void
CDisplay::NoteMost(CLine *pli)
{
    Assert (pli);

    if ( !_fRecalcMost && (pli->GetYMostTop() < 0 || pli->_fHasAbsoluteElt))
    {
        _fRecalcMost = TRUE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     RecalcMost
//
//  Purpose:    Calculates the most negative line and most positive/negative
//              positioned site from scratch.
//
//  BUGBUG(sujalp): We initially had an incremental way of computing both the
//  negative line hts info AND +/- positioned site info. However, this logic was
//  incorrect for consecutive lines with -ve line height. So we changed it so
//  that we compute this AND +/- info always. If this becomes a performance issue
//  we could go back for incremental computation for div's easily -- but would
//  have to maintain extra state information. For negative line heights we could
//  also do some incremental stuff, but it would be much much more complicated
//  than what we have now.
//
//----------------------------------------------------------------------------
void
CDisplay::RecalcMost()
{
    if (_fRecalcMost)
    {
        CFlowLayout * pFlowLayout = GetFlowLayout();
        LONG ili;

        long yNegOffset = 0;        // offset at which the current line is drawn
                                    // as a result of a series of lines with negative
                                    // height
        long yPosOffset = 0;

        long yBottomOffset = 0;     // offset by which the current lines contents
                                    // extend beyond the yHeight of the line.
        long yTopOffset = 0;        // offset by which the current lines contents
                                    // extend before the current y position

        pFlowLayout->_fContainsAbsolute = FALSE;
        _yMostNeg = 0;
        _yMostPos = 0;

        for (ili = 0; ili < LineCount(); ili++)
        {
            CLine *pli = Elem (ili);
            LONG   yLineBottomOffset = pli->GetYHeightBottomOff();

            // Update most positive/negative positions for relative positioned sites
            if (pli->_fHasAbsoluteElt)
                pFlowLayout->_fContainsAbsolute = FALSE;

            // top offset of the current line
            yTopOffset = pli->GetYMostTop() + yNegOffset;

            yBottomOffset = yLineBottomOffset + yPosOffset;

            // update the most negative value if the line has negative before space
            // or line height < actual extent
            if(yTopOffset < 0 && _yMostNeg > yTopOffset)
            {
                _yMostNeg = yTopOffset;
            }

            if (yBottomOffset > 0 && _yMostPos < yBottomOffset)
            {
                _yMostPos = yBottomOffset;
            }

            // if the current line forces a new line and has negative height
            // update the negative offset at which the next line is drawn.
            if(pli->_fForceNewLine)
            {
                if(pli->_yHeight < 0)
                {
                    yNegOffset += pli->_yHeight;
                }
                else
                {
                    yNegOffset = 0;
                }

                if (yLineBottomOffset > 0)
                {
                    yPosOffset += yLineBottomOffset;
                }
                else
                {
                    yPosOffset = 0;
                }
            }
        }

        _fRecalcMost = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     RecalcPlainTextSingleLine
//
//  Purpose:    A high-performance substitute for RecalcView. Does not go
//              through Line Services. Can only be used to measure a single
//              line of plain text (i.e. no HTML).
//
//----------------------------------------------------------------------------

BOOL
CDisplay::RecalcPlainTextSingleLine(CCalcInfo * pci)
{
    CFlowLayout *       pFlowLayout = GetFlowLayout();
    CTreeNode *         pNode       = pFlowLayout->GetFirstBranch();
    TCHAR               chPassword  = pFlowLayout->GetPasswordCh();
    long                cch         = pFlowLayout->GetContentTextLength();
    const CCharFormat * pCF         = pNode->GetCharFormat();
    const CParaFormat * pPF         = pNode->GetParaFormat();
    CCcs *              pccs = NULL;
    CBaseCcs *          pBaseCcs = NULL;
    long                lWidth;
    long                lCharWidth;
    long                xShift;
    long                xWidth, yHeight;
    long                lPadding[PADDING_MAX];
    BOOL                fViewChanged = FALSE;
    CLine *             pli;
    UINT                uJustified;
    long                xDummy, yBottomMarginOld = _yBottomMargin;


    Assert(pPF);
    Assert(pCF);
    Assert(pci);
    Assert(cch >= 0);

    if (!pPF || !pCF || !pci || cch < 0)
        return FALSE;

    // Bail out if there is anything special in the format
    // BUGBUG (MohanB) Should be able to handle line height. For now though,
    // do this to keep RoboVision happy (with lheight.htm).
    // (paulnel) Right to left text cannot be done the 'quick' way
    if (    pCF->IsTextTransformNeeded()
        ||  !pCF->_cuvLineHeight.IsNullOrEnum()
        ||  !pCF->_cuvLetterSpacing.IsNullOrEnum()
        ||  pCF->_fRTL)
    {
        goto HardCase;
    }


    pccs = fc().GetCcs(pci->_hdc, pci, pCF);
    if (!pccs)
        return FALSE;
    pBaseCcs = pccs->GetBaseCcs();
    
    lWidth = 0;

    if (cch)
    {
        if (chPassword)
        {
            if (!pccs->Include(chPassword, lCharWidth) )
            {
                Assert(0 && "Char not in font!");
            }
            lWidth = cch * lCharWidth;
        }
        else
        {
            CTxtPtr     tp(GetMarkup(), pFlowLayout->GetContentFirstCp());
            LONG        cchValid;
            LONG        cchRemaining = cch;

            for (;;)
            {
                const TCHAR * pchText = tp.GetPch(cchValid);
                LONG i = min(cchRemaining, cchValid);

                while (i--)
                {
                    const TCHAR ch = *pchText++;

                    // Bail out if not a simple ASCII char

                    if (!InRange( ch, 32, 127 ))
                        goto HardCase;

                    if (!pccs->Include(ch, lCharWidth))
                    {
                        Assert(0 && "Char not in font!");
                    }
                    lWidth += lCharWidth;
                }

                if (cchRemaining <= cchValid)
                {
                    break;
                }
                else
                {
                    cchRemaining -= cchValid;
                    tp.AdvanceCp(cchValid);
                }
            }
        }
    }

    GetPadding(pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH);
    FlushRecalc();

    pli = Add(1, NULL);
    if (!pli)
        return FALSE;

    pli->_cch               = cch;
    pli->_xWidth            = lWidth;
    pli->_yTxtDescent       = pBaseCcs->_yDescent;
    pli->_yDescent          = pBaseCcs->_yDescent;
    pli->_xLineOverhang     = pBaseCcs->_xOverhang;
    pli->_yExtent           = pBaseCcs->_yHeight;
    pli->_yBeforeSpace      = lPadding[PADDING_TOP];
    pli->_yHeight           = pBaseCcs->_yHeight + pli->_yBeforeSpace;
    pli->_xLeft             = lPadding[PADDING_LEFT];
    pli->_xRight            = lPadding[PADDING_RIGHT];
    pli->_xLeftMargin       = 0;
    pli->_xRightMargin      = 0;
    pli->_fForceNewLine     = TRUE;
    pli->_fFirstInPara      = TRUE;
    pli->_fFirstFragInLine  = TRUE;
    pli->_fCanBlastToScreen = !chPassword && !pCF->_fDisabled;

    _yBottomMargin  = lPadding[PADDING_BOTTOM];
    _dcpCalcMax     = cch;

    yHeight          = pli->_yHeight;
    xWidth           = pli->_xLeft + pli->_xWidth + pli->_xLineOverhang + pli->_xRight;

    Assert(pci->_smMode != SIZEMODE_MINWIDTH);

    xShift = ComputeLineShift(
                        (htmlAlign)pPF->GetBlockAlign(TRUE),
                        _fRTL,
                        pPF->HasRTL(TRUE),
                        pci->_smMode == SIZEMODE_MMWIDTH,
                        _xWidthView,
                        xWidth + GetCaret(),
                        &uJustified,
                        &xDummy);

    pli->_fJustified = uJustified;

    if(!_fRTL)
        pli->_xLeft += xShift;
    else
        pli->_xRight += xShift;

    xWidth      += xShift;

    pli->_xLineWidth = max(xWidth, _xWidthView);

    if(yHeight + yBottomMarginOld != _yHeight + _yBottomMargin || xWidth != _xWidth)
        fViewChanged = TRUE;

    _yCalcMax       =
    _yHeightMax     =
    _yHeight        = yHeight;
    _xWidth         = xWidth;
    _fRecalcDone    = TRUE;
    _fMinMaxCalced  = TRUE;
    _xMinWidth      =
    _xMaxWidth      = _xWidth + GetCaret();

    if (pci->_smMode == SIZEMODE_NATURAL && fViewChanged)
    {
        pFlowLayout->SizeContentDispNode(CSize(GetMaxWidth(), GetHeight()));

        // If our contents affects our size, ask our parent to initiate a re-size
        ElementResize(pFlowLayout);

        pFlowLayout->NotifyMeasuredRange(pFlowLayout->GetContentFirstCp(),
                                         GetMaxCpCalced());
    }

    pccs->Release();

    return TRUE;

HardCase:

    if (pccs)
    {
        pccs->Release();
    }

    // Just do it the hard way
    return RecalcLines(pci);
}

/*
 *  CDisplay::RecalcLines()
 *
 *  @mfunc
 *      Recalc all line breaks.
 *      This method does a lazy calc after the last visible line
 *      except for a bottomless control
 *
 *  @rdesc
 *      TRUE if success
 */

BOOL CDisplay::RecalcLines(CCalcInfo * pci)
{
#ifdef SWITCHES_ENABLED
    if (IsSwitchNoRecalcLines())
        return FALSE;
#endif

    SwitchesBegTimer(SWITCHES_TIMER_RECALCLINES);

    BOOL fRet;

    if (GetFlowLayout()->ElementOwner()->IsInMarkup())
    {
        if (        pci->_fTableCalcInfo
                    && ((CTableCalcInfo *) pci)->_pme
          WHEN_DBG( && !IsTagEnabled(tagTableCalcDontReuseMeasurer) ) )
        {
            // Save calcinfo's measurer.
            CTableCalcInfo * ptci = (CTableCalcInfo *) pci;
            CLSMeasurer * pme = ptci->_pme;

            // Reinitialize the measurer.
            pme->Reinit(this, ptci);

            // Make sure noone else uses this measurer.
            ptci->_pme = NULL;

            // Do actual RecalcLines work with this measurer.
            fRet = RecalcLinesWithMeasurer(ptci, pme);

            // Restore TableCalcInfo measurer.
            ptci->_pme = pme;
        }
        else
        {
            // Cook up measurer on the stack.
            CLSMeasurer me(this, pci);

            fRet = RecalcLinesWithMeasurer(pci, &me);
        }
    }
    else
        fRet = FALSE;

    SwitchesEndTimer(SWITCHES_TIMER_RECALCLINES);

    return fRet;
}


/*
 *  CDisplay::RecalcLines()
 *
 *  @mfunc
 *      Recalc all line breaks.
 *      This method does a lazy calc after the last visible line
 *      except for a bottomless control
 *
 *  @rdesc
 *      TRUE if success
 */

BOOL CDisplay::RecalcLinesWithMeasurer(CCalcInfo * pci, CLSMeasurer * pme)
{
#ifdef SWITCHES_ENABLED
    if (IsSwitchNoRecalcLines())
        return FALSE;
#endif

    CFlowLayout * pFlowLayout = GetFlowLayout();
    CElement    * pElementFL  = pFlowLayout->ElementOwner();
    CElement::CLock     Lock(pElementFL, CElement::ELEMENTLOCK_RECALC);

    CRecalcLinePtr  RecalcLinePtr(this, pci);
    CLine *         pliNew    = NULL;
    int             iLine     = -1;
    int             iLinePrev = -1;
    long            yHeightPrev;
    UINT            uiMode;
    UINT            uiFlags;
    BOOL            fDone                   = TRUE;
    BOOL            fFirstInPara            = TRUE;
    BOOL            fWaitForCpToBeCalcedTo  = TRUE;
    LONG            cpToBeCalcedTo          = 0;
    BOOL            fAllowBackgroundRecalc;
    LONG            yHeight         = 0;
    LONG            yAlignDescent   = 0;
    LONG            yHeightView     = GetViewHeight();
    LONG            xMinLineWidth   = 0;
    LONG *          pxMinLineWidth  = NULL;
    LONG            xMaxLineWidth   = 0;
    LONG            yHeightDisplay  = 0;     // to keep track of line with top negative margins
    LONG            yHeightOld      = _yHeight;
    LONG            yHeightMaxOld   = _yHeightMax;
    LONG            xWidthOld       = _xWidth;
    LONG            yBottomMarginOld= _yBottomMargin;
    BOOL            fViewChanged    = FALSE;
    BOOL            fNormalMode     =   pci->_smMode == SIZEMODE_NATURAL
                                    ||  pci->_smMode == SIZEMODE_SET
                                    ||  pci->_smMode == SIZEMODE_FULLSIZE;
    CDispNode *     pDNBefore;
    long            lPadding[PADDING_MAX];

    // we should not be measuring hidden stuff.
    Assert(!pElementFL->IsDisplayNone() || pElementFL->Tag() == ETAG_BODY);

    if (pElementFL->Tag() == ETAG_ROOT)
        return TRUE;

    if (!pme->_pLS)
        return FALSE;

    if (!pElementFL->IsInMarkup())
    {
        return TRUE;
    }

    GetPadding(pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH);

    _yBottomMargin = lPadding[PADDING_BOTTOM];

    // Set up the CCalcInfo to the correct mode and parent size
    if (pci->_smMode == SIZEMODE_SET)
    {
        pci->_smMode = SIZEMODE_NATURAL;
    }

    // Determine the mode
    uiMode = MEASURE_BREAKATWORD;

    Assert (pci);

    switch(pci->_smMode)
    {
    case SIZEMODE_MMWIDTH:
        uiMode = MEASURE_BREAKATWORD | MEASURE_MAXWIDTH;
        _xMinWidth = 0;
        pxMinLineWidth = &xMinLineWidth;
        break;
    case SIZEMODE_MINWIDTH:
        uiMode = MEASURE_BREAKATWORD | MEASURE_MINWIDTH;
        pme->AdvanceToNextAlignedObject();
        break;
    }

    uiFlags = uiMode;

    // Determine if background recalc is allowed
    // (And if it is, do not calc past the visible portion)
    fAllowBackgroundRecalc = AllowBackgroundRecalc(pci);

    if (fAllowBackgroundRecalc)
    {
        cpToBeCalcedTo = max(GetFirstVisibleCp(), CPWait());
    }
    
    // Flush all old lines
    FlushRecalc();

    if ( GetWordWrap() && GetWrapLongLines() )
    {
        uiFlags |= MEASURE_BREAKLONGLINES;
    }

    pme->_pDispNodePrev = NULL;

    if (fAllowBackgroundRecalc)
    {
        if (!SUCCEEDED(EnsureBgRecalcInfo()))
        {
            fAllowBackgroundRecalc = FALSE;
            AssertSz(FALSE, "CDisplay::RecalcLinesWithMeasurer - Could not create BgRecalcInfo");
        }
    }

    RecalcLinePtr.Init((CLineArray *)this, 0, NULL);

    // recalculate margins
    RecalcLinePtr.RecalcMargins(0, 0, yHeight, 0);

    if (pElementFL->IsDisplayNone())
    {
        AssertSz(pElementFL->Tag() == ETAG_BODY, "Cannot measure any hidden layout other than the body");

        pliNew = RecalcLinePtr.AddLine();
        if (!pliNew)
        {
            Assert(FALSE);
            goto err;
        }
        pme->NewLine(uiFlags & MEASURE_FIRSTINPARA);
        *pliNew = pme->_li;
        pliNew->_cch = pElementFL->GetElementCch();
    }
    else
    {
        // The following loop generates new lines
        do
        {
            // Add one new line
            pliNew = RecalcLinePtr.AddLine();
            if (!pliNew)
            {
                Assert(FALSE);
                goto err;
            }

            uiFlags &= ~MEASURE_FIRSTINPARA;
            uiFlags |= (fFirstInPara ? MEASURE_FIRSTINPARA : 0);

            PerfDbgLog1(tagRecalc, this, "Measuring new line from cp = %d", pme->GetCp());

            iLine = LineCount() - 1;

            if (long(pme->GetCp()) == pme->GetLastCp())
            {
                uiFlags |= MEASURE_EMPTYLASTLINE;
            }

            //
            // Cache the previous dispnode to insert a new content node
            // if the next line measured has negative margins(offset) which
            // causes the next line on top of the previous lines.
            //
            pDNBefore = pme->_pDispNodePrev;
            iLinePrev = iLine;
            yHeightPrev = yHeight;

            if(!RecalcLinePtr.MeasureLine(*pme, uiFlags,
                                            &iLine, &yHeight, &yAlignDescent,
                                            pxMinLineWidth, &xMaxLineWidth))
            {
                goto err;
            }

            // fix for bug 16519 (srinib)
            // Keep track of the line that contributes to the max height, ex: if the
            // last lines top margin is negative.
            if(yHeightDisplay < yHeight)
                yHeightDisplay = yHeight;

            //
            // iLine returned is the last text line inserted. There ay be
            // aligned lines and clear lines added before and after the
            // text line
            //
            pliNew = iLine >=0 ? RecalcLinePtr[iLine] : NULL;
            fFirstInPara = (iLine >= 0)
                                ? pliNew->IsNextLineFirstInPara()
                                : TRUE;

            if (fNormalMode && iLine >= 0)
            {
                HandleNegativelyPositionedContent(pliNew, pme, pDNBefore, iLinePrev, yHeightPrev);
            }

            if (fAllowBackgroundRecalc)
            {
                Assert(HasBgRecalcInfo());

                if (fWaitForCpToBeCalcedTo)
                {
                    if ((LONG)pme->GetCp() > cpToBeCalcedTo)
                    {
                        BgRecalcInfo()->_yWait  = yHeight + yHeightView;
                        fWaitForCpToBeCalcedTo = FALSE;
                    }
                }

                else if (yHeightDisplay > YWait())
                {
                    fDone = FALSE;
                    break;
                }
            }

            // When doing a full min pass, just calculate the islands around aligned
            // objects. Note that this still calc's the lines around right aligned
            // images, which isn't really necessary because they are willing to
            // overlap text. Fix it next time.
            if ((uiMode & MEASURE_MINWIDTH) &&
                !RecalcLinePtr._marginInfo._xLeftMargin &&
                !RecalcLinePtr._marginInfo._xRightMargin )
                pme->AdvanceToNextAlignedObject();

        } while( pme->GetCp() < pme->GetLastCp());
    }
    
    _fRecalcDone = fDone;

    if(fDone && pliNew)
    {
        if (GetFlowLayout()->IsEditable() && // if we are in design mode
                (pliNew->_fHasEOP || pliNew->_fHasBreak || pliNew->_fSingleSite))
        {
            Assert(pliNew == RecalcLinePtr[iLine]);
            CreateEmptyLine(pme, &RecalcLinePtr, &yHeight, pliNew->_fHasEOP );

            // Creating the line could have reallocated the memory to which pliNew points,
            // so we need to refresh the pointer just in case.

            pliNew = RecalcLinePtr[iLine];
        }

        // fix for bug 16519
        // Keep track of the line that contributes to the max height, ex: if the
        // last lines top margin is negative.
        if(yHeightDisplay < yHeight)
            yHeightDisplay = yHeight;

        // In table cells, Netscape actually adds the interparagraph spacing
        // for any closed tags to the height of the table cell.
        // BUGBUG: This actually applies the spacing to all nested text containers, not just
        //         table cells. Is this correct? (brendand).
        // It's not correct to add the spacing at all, but given that Netscape
        // has done so, and that they will probably do so for floating block
        // elements. Arye.
        else
        {
            int iLineLast = iLine;
            
            // we need to force scrolling when bottom-margin is set on the last block tag
            // in body. (20400)

            while (iLineLast-- > 0 &&   // Lines available
                   !pliNew->_fForceNewLine)   // Just a dummy line.
                --pliNew;
            if (iLineLast > 0 || pliNew->_fForceNewLine)
                _yBottomMargin += RecalcLinePtr.NetscapeBottomMargin(pme);
        }
    }

    if (!(uiMode & MEASURE_MAXWIDTH))
    {
        xMaxLineWidth = CalcDisplayWidth();
    }

    _dcpCalcMax = (LONG)pme->GetCp() - GetFirstCp();
    _yCalcMax   = 
    _yHeight    = yHeightDisplay;
    _yHeightMax = max(yHeightDisplay, yAlignDescent);
    _xWidth     = xMaxLineWidth;

    //
    // Fire a resize notification if the content size changed
    //  
    // fix for 52305, change to margin bottom affects the content size,
    // so we should fire a resize.
    //
    if(     _yHeight    != yHeightOld
        ||  _yHeightMax + _yBottomMargin != yHeightMaxOld + yBottomMarginOld
        ||  _xWidth     != xWidthOld)
    {
        fViewChanged = TRUE;
    }

    {
        BOOL fAlignedLayouts =    RecalcLinePtr._cLeftAlignedLayouts
                               || (RecalcLinePtr._cRightAlignedLayouts > 1);
        if(pxMinLineWidth)
        {
            pFlowLayout->MarkHasAlignedLayouts(fAlignedLayouts);
        }
        if (fAlignedLayouts)
            _fNoContent = FALSE;
    }

    // If the view or size changed, re-size or update scrollbars as appropriate
    // BUGBUG: Collapse both RecalcLines into one and remove knowledge of
    //         CSite::SIZEMODE_xxxx (it should be based entirely on the
    //         passed MEASURE_xxxx flags set in CalcSize) (brendand)
    if (pci->_smMode == SIZEMODE_NATURAL && (fViewChanged || _fHasMultipleTextNodes))
    {
        pFlowLayout->SizeContentDispNode(CSize(GetMaxWidth(), GetHeight()));

        // If our contents affects our size, ask our parent to initiate a re-size
        if ( fViewChanged )
            ElementResize(pFlowLayout);
    }

    if (fNormalMode)
    {
        if (pFlowLayout->_fContainsRelative)
            UpdateRelDispNodeCache(NULL);

        AdjustDispNodes(pme->_pDispNodePrev, NULL);

        pFlowLayout->NotifyMeasuredRange(GetFlowLayoutElement()->GetFirstCp(), GetMaxCpCalced());
    }

    PerfDbgLog1(tagRecalc, this, "CDisplay::RecalcLine() - Done. Recalced down to line #%d", LineCount());

    if(!fDone)                      // if not done, do rest in background
    {
        StartBackgroundRecalc(pci->_grfLayout);
    }
    else if (fAllowBackgroundRecalc)
    {
        Assert((!CanHaveBgRecalcInfo() || BgRecalcInfo()) && "Should have a BgRecalcInfo");
        if (HasBgRecalcInfo())
        {
            CBgRecalcInfo * pBgRecalcInfo = BgRecalcInfo();
            pBgRecalcInfo->_yWait = -1;
            pBgRecalcInfo->_cpWait = -1;
        }
#if DBG==1
        if (!(uiMode & MEASURE_MINWIDTH))
            CheckLineArray();
#endif
        _fLineRecalcErr = FALSE;

        RecalcMost();
    }

    // cache min/max only if there are no sites inside !
    if (pxMinLineWidth && !pFlowLayout->ContainsChildLayout())
    {
        _xMaxWidth      = _xWidth + GetCaret();
        _fMinMaxCalced  = TRUE;
    }

    // adjust for caret only when are calculating for MMWIDTH or MINWIDTH
    if (pci->_smMode == SIZEMODE_MMWIDTH || pci->_smMode == SIZEMODE_MINWIDTH)
    {
        if (pci->_smMode == SIZEMODE_MMWIDTH)
            _xMinWidth = max(_xMinWidth, RecalcLinePtr._xMaxRightAlign);
        _xMinWidth      += GetCaret();  // adjust for caret only when are calculating for MMWIDTH
    }

    // NETSCAPE: If there is no text or special characters, treat the site as
    //           empty. For example, with an empty BLOCKQUOTE tag, _xWidth will
    //           not be zero while the site should be considered empty.
    if(_fNoContent)
    {
        _xWidth =
        _xMinWidth = lPadding[PADDING_LEFT] + lPadding[PADDING_RIGHT];
    }

    return TRUE;

err:
    TraceTag((tagError, "CDisplay::RecalcLines() failed"));

    if(!_fLineRecalcErr)
    {
        _dcpCalcMax   = pme->GetCp() - GetFirstCp();
        _yCalcMax = yHeightDisplay;
    }

    return FALSE;
}

/*
 *  CDisplay::RecalcLines(cpFirst, cchOld, cchNew, fBackground, pled)
 *
 *  @mfunc
 *      Recompute line breaks after text modification
 *
 *  @rdesc
 *      TRUE if success
 */


BOOL CDisplay::RecalcLines (
    CCalcInfo * pci,
    LONG cpFirst,               //@parm Where change happened
    LONG cchOld,                //@parm Count of chars deleted
    LONG cchNew,                //@parm Count of chars added
    BOOL fBackground,           //@parm This method called as background process
    CLed *pled,                 //@parm Records & returns impact of edit on lines (can be NULL)
    BOOL fHack)                 //@parm This comes from WaitForRecalc ... don't mess with BgRecalcInfo
{
#ifdef SWITCHES_ENABLED
    if (IsSwitchNoRecalcLines())
        return FALSE;
#endif

    CSaveCalcInfo       sci(pci);
    CElement::CLock     Lock(GetFlowLayoutElement(), CElement::ELEMENTLOCK_RECALC);

    CFlowLayout * pFlowLayout = GetFlowLayout();
    CElement    * pElementContent  = pFlowLayout->ElementContent();

    LONG        cchEdit;
    LONG        cchSkip = 0;
    INT         cliBackedUp = 0;
    LONG        cliWait = g_cExtraBeforeLazy;
    BOOL        fDone = TRUE;
    BOOL        fFirstInPara = TRUE;
    BOOL        fAllowBackgroundRecalc;
    CLed        led;
    LONG        lT = 0;         // long Temporary
    CLine *     pliNew;
    CLinePtr    rpOld(this);
    CLine *     pliCur;
    LONG        xWidth;
    LONG        yHeight, yAlignDescent=0;
    LONG        cpLast = GetLastCp();
    long        cpLayoutFirst = GetFirstCp();
    UINT        uiFlags = MEASURE_BREAKATWORD;
    BOOL        fReplaceResult = TRUE;
    BOOL        fTryForMatch = TRUE;
    BOOL        fNeedToBackUp = TRUE;
    int         diNonBlankLine = -1;
    int         iOldLine;
    int         iNewLine;
    int         iLinePrev = -1;
    int         iMinimumLinesToRecalc = 4;   // Guarantee some progress
    LONG        yHeightDisplay = 0;
    LONG        yHeightMax;
    CLineArray  aryNewLines;
    CDispNode * pDNBefore;
    long        yHeightPrev;
    long        yBottomMarginOld = _yBottomMargin;

#if DBG==1
    LONG        cp;
#endif

    if (!pFlowLayout->ElementOwner()->IsInMarkup())
    {
        return TRUE;
    }

    SwitchesBegTimer(SWITCHES_TIMER_RECALCLINES);

    // we should not be measuring hidden stuff.
    Assert(!pFlowLayout->ElementOwner()->IsDisplayNone() || pFlowLayout->ElementOwner()->Tag() == ETAG_BODY);

    // If no lines, this routine should not be called
    // Call the other RecalcLines above instead !
    Assert(LineCount() > 0);

    // Set up the CCalcInfo to the correct mode and parent size
    if (pci->_smMode == SIZEMODE_SET)
    {
        pci->_smMode = SIZEMODE_NATURAL;
    }

    // Init measurer at cpFirst
    CLSMeasurer     me(this, cpFirst, pci);

    CRecalcLinePtr  RecalcLinePtr(this, pci);

    if (!me._pLS)
        goto err;

    if (!pled)
        pled = &led;

#if DBG==1
    if(cpFirst > GetMaxCpCalced())
        TraceTag((tagError, "cpFirst %ld, CalcMax %ld", cpFirst, GetMaxCpCalced()));

    AssertSz(cpFirst <= GetMaxCpCalced(),
        "CDisplay::RecalcLines Caller didn't setup RecalcLines()");
#endif

    if (    GetWordWrap()
        &&  GetWrapLongLines())
    {
        uiFlags |= MEASURE_BREAKLONGLINES;
    }

    // Determine if background recalc is allowed
    fAllowBackgroundRecalc = AllowBackgroundRecalc(pci, fBackground);

    Assert(!fBackground || HasBgRecalcInfo());
    if (    !fBackground
        &&  !fHack
        &&  fAllowBackgroundRecalc)
    {
        if (SUCCEEDED(EnsureBgRecalcInfo()))
        {
            BgRecalcInfo()->_yWait  = pFlowLayout->GetClientBottom();
            BgRecalcInfo()->_cpWait = -1;
        }
        else
        {
            fAllowBackgroundRecalc = FALSE;
            AssertSz(FALSE, "CDisplay::RecalcLines - Could not create BgRecalcInfo");
        }
    }

    // Init line pointer on old CLineArray
    rpOld.RpSetCp(cpFirst, FALSE, FALSE);

    pliCur = rpOld.CurLine();

    while(pliCur->IsClear() ||
          (pliCur->IsFrame() && !pliCur->_fFrameBeforeText))
    {
        if(!rpOld.AdjustBackward())
            break;

        pliCur = rpOld.CurLine();
    }

    // If the current line has single site
    if(pliCur->_fSingleSite)
    {
        // If we are in the middle of the current line (some thing changed
        // in a table cell or 1d-div, or marquee)
        if(rpOld.RpGetIch() && rpOld.GetCchRemaining() != 0)
        {
            // we dont need to back up
            if(rpOld > 0 && rpOld[-1]._fForceNewLine)
            {
                fNeedToBackUp = FALSE;
                cchSkip = rpOld.RpGetIch();
            }
        }
    }

    iOldLine = rpOld;

    if(fNeedToBackUp)
    {
        if(!rpOld->IsBlankLine())
        {
            cchSkip = rpOld.RpGetIch();
            if(cchSkip)
                rpOld.RpAdvanceCp(-cchSkip);
        }

        // find the first previous non blank line, if the first non blank line
        // is a single site line or a line with EOP, we don't need to skip.
        while(rpOld + diNonBlankLine >= 0 &&
                rpOld[diNonBlankLine].IsBlankLine())
            diNonBlankLine--;

        // (srinib) - if the non text line does not have a line break or EOP, or
        // a force newline or is single site line and the cp at which the characters changed is
        // ambiguous, we need to back up.

        // if the single site line does not force a new line, we do need to
        // back up. (bug #44346)
        if(rpOld + diNonBlankLine >= 0)
        {
            pliCur = &rpOld[diNonBlankLine];
            if(!pliCur->_fSingleSite || !pliCur->_fForceNewLine || cchSkip == 0)
            {
                // Skip past all the non text lines
                while(diNonBlankLine++ < 0)
                    rpOld.AdjustBackward();

                // Skip to the begining of the line
                cchSkip += rpOld.RpGetIch();
                rpOld.RpAdvanceCp(-rpOld.RpGetIch());

                // we want to skip all the lines that do not force a
                // newline, so find the last line that does not force a new line.
                long diTmpLine = -1;
                long cchSkipTemp = 0;
                while(rpOld + diTmpLine >=0 &&
                        ((pliCur = &rpOld[diTmpLine]) != 0) &&
                        !pliCur->_fForceNewLine)
                {
                    if(!pliCur->IsBlankLine())
                        cchSkipTemp += pliCur->_cch;
                    diTmpLine--;
                }
                if(cchSkipTemp)
                {
                    cchSkip += cchSkipTemp;
                    rpOld.RpAdvanceCp(-cchSkipTemp);
                }
            }
        }

        // back up further if the previous lines are either frames inserted
        // by aligned sites at the beginning of the line or auto clear lines
        while(rpOld && ((pliCur = &rpOld[-1]) != 0) &&
              // frame line before the actual text or
              (pliCur->_fClearBefore ||
               (pliCur->IsFrame() && pliCur->_fFrameBeforeText)) &&
              rpOld.AdjustBackward());
    }

    cliBackedUp = iOldLine - rpOld;

    // Need to get a running start.
    me.Advance(-cchSkip);
    RecalcLinePtr.InitPrevAfter(&me._fLastWasBreak, rpOld);

    cchEdit = cchNew + cchSkip;         // Number of chars affected by edit

    Assert(cchEdit <= GetLastCp() - long(me.GetCp()) );
    if (cchEdit > GetLastCp() - long(me.GetCp()))
    {
        // BUGBUG (istvanc) this of course shouldn't happen !!!!!!!!
        cchEdit = GetLastCp() - me.GetCp();
    }

    // Determine whether we're on first line of paragraph
    if(rpOld > 0)
    {
        int iLastNonFrame = -1;

        // frames do not hold any info, so go back past all frames.
        while(rpOld + iLastNonFrame && (rpOld[iLastNonFrame].IsFrame() || rpOld[iLastNonFrame].IsClear()))
            iLastNonFrame--;
        if(rpOld + iLastNonFrame >= 0)
        {
            CLine *pliNew = &rpOld[iLastNonFrame];

            fFirstInPara = pliNew->IsNextLineFirstInPara();
        }
        else
        {
            // we are at the Beginning of a document
            fFirstInPara = TRUE;
        }
    }

    yHeight = YposFromLine(rpOld, pci);
    yHeightDisplay = yHeight;
    yAlignDescent = 0;

    // Update first-affected and pre-edit-match lines in pled
    pled->_iliFirst = rpOld;
    pled->_cpFirst  = pled->_cpMatchOld = me.GetCp();
    pled->_yFirst   = pled->_yMatchOld  = yHeight;

    //
    // In the presence of negative margins and negative line heights, its no
    // longer possible to verify the following statement (bug 28255).
    // I have also verified that it being negative does not cause any other
    // problems in the code that utilizes its value (SujalP).
    //
    // AssertSz(pled->_yFirst >= 0, "CDisplay::RecalcLines _yFirst < 0");

    PerfDbgLog2(tagRecalc, this, "Start recalcing from line #%d, cp=%d",
              pled->_iliFirst, pled->_cpFirst);

    // In case of error, set both maxes to where we are now
    _yCalcMax   = yHeight;
    _dcpCalcMax = me.GetCp() - cpLayoutFirst;

    //
    // find the display node the corresponds to cpStart
    //
    me._pDispNodePrev = pled->_iliFirst
                            ? GetPreviousDispNode(pled->_cpFirst, pled->_iliFirst)
                            : 0;


    // If we are past the requested area to recalc and background recalc is
    // allowed, then just go directly to background recalc.
    if (    fAllowBackgroundRecalc
        &&  yHeight > YWait()
        &&  (LONG) me.GetCp() > CPWait())
    {
        // Remove all old lines from here on
        rpOld.RemoveRel(-1, AF_KEEPMEM);

        // Start up the background recalc
        StartBackgroundRecalc(pci->_grfLayout);
        pled->SetNoMatch();

        // Update the relative line cache.
        if (pFlowLayout->_fContainsRelative)
            UpdateRelDispNodeCache(NULL);

        AdjustDispNodes(me._pDispNodePrev, pled);

        goto cleanup;
    }

    aryNewLines.Clear(AF_KEEPMEM);
    pliNew = NULL;

    iOldLine = rpOld;
    RecalcLinePtr.Init((CLineArray *)this, iOldLine, &aryNewLines);

    // recalculate margins
    RecalcLinePtr.RecalcMargins(iOldLine, iOldLine, yHeight, 0);

    Assert ( cchEdit <= GetLastCp() - long(me.GetCp()) );

    if(iOldLine)
        RecalcLinePtr.SetupMeasurerForBeforeSpace(&me);

    // The following loop generates new lines for each line we backed
    // up over and for lines directly affected by edit
    while(cchEdit > 0)
    {
        LONG iTLine;
        LONG cpTemp;
        pliNew = RecalcLinePtr.AddLine();       // Add one new line
        if (!pliNew)
        {
            Assert(FALSE);
            goto err;
        }

        // Stuff text into new line
        uiFlags &= ~MEASURE_FIRSTINPARA;
        uiFlags |= (fFirstInPara ? MEASURE_FIRSTINPARA : 0);

        PerfDbgLog1(tagRecalc, this, "Measuring new line from cp = %d", me.GetCp());

        // measure can add lines for aligned sites
        iNewLine = iOldLine + aryNewLines.Count() - 1;

        // save the index of the new line to added
        iTLine = iNewLine;

#if DBG==1
        {
            // Just so we can see the damn text.
            const TCHAR *pch;
            long cchInStory = (long)GetFlowLayout()->GetContentTextLength();
            long cp = (long)me.GetCp();
            long cchRemaining =  cchInStory - cp;
            CTxtPtr tp(GetMarkup(), cp);
            pch = tp.GetPch(cchRemaining);
#endif

        cpTemp = me.GetCp();

        if (cpTemp == pFlowLayout->GetContentLastCp() - 1)
        {
            uiFlags |= MEASURE_EMPTYLASTLINE;
        }

        //
        // Cache the previous dispnode to insert a new content node
        // if the next line measured has negative margins(offset) which
        // causes the next line on top of the previous lines.
        //
        pDNBefore = me._pDispNodePrev;
        iLinePrev = iNewLine;
        yHeightPrev = yHeight;

        if (!RecalcLinePtr.MeasureLine(me, uiFlags,
                                       &iNewLine, &yHeight, &yAlignDescent,
                                       NULL, NULL))
        {
            goto err;
        }

        //
        // iNewLine returned is the last text line inserted. There ay be
        // aligned lines and clear lines added before and after the
        // text line
        //
        pliNew = iNewLine >=0 ? RecalcLinePtr[iNewLine] : NULL;

        fFirstInPara = (iNewLine >= 0)
                            ? pliNew->IsNextLineFirstInPara()
                            : TRUE;
#if DBG==1
        }
#endif
        // fix for bug 16519 (srinib)
        // Keep track of the line that contributes to the max height, ex: if the
        // last lines top margin is negative.
        if(yHeightDisplay < yHeight)
            yHeightDisplay = yHeight;

        if (iNewLine >= 0)
        {
            HandleNegativelyPositionedContent(pliNew, &me, pDNBefore, iLinePrev, yHeightPrev);
        }

        // If we have added any clear lines, iNewLine points to a textline
        // which could be < iTLine
        if(iNewLine >= iTLine)
            cchEdit -= me.GetCp() - cpTemp;

        if(cchEdit <= 0)
            break;

        --iMinimumLinesToRecalc;
        if(iMinimumLinesToRecalc < 0 &&
           fBackground && (GetTickCount() >= BgndTickMax())) // GetQueueStatus(QS_ALLEVENTS))
        {
            fDone = FALSE;                      // took too long, stop for now
            goto no_match;
        }

        if (    fAllowBackgroundRecalc
            &&  yHeight > YWait()
            &&  (LONG) me.GetCp() > CPWait()
            &&  cliWait-- <= 0)
        {
            // Not really done, just past region we're waiting for
            // so let background recalc take it from here
            fDone = FALSE;
            goto no_match;
        }
    }                                           // while(cchEdit > 0) { }

    PerfDbgLog1(tagRecalc, this, "Done recalcing edited text. Created %d new lines", aryNewLines.Count());

    // Edit lines have been exhausted.  Continue breaking lines,
    // but try to match new & old breaks

    while( (LONG) me.GetCp() < cpLast )
    {
        // Assume there are no matches to try for
        BOOL frpOldValid = FALSE;
        BOOL fChangedLineSpacing = FALSE;

        // If we run out of runs, then there is not match possible. Therefore,
        // we only try for a match as long as we have runs.
        if (fTryForMatch)
        {
            // We are trying for a match so assume that there
            // is a match after all
            frpOldValid = TRUE;

            // Look for match in old line break CArray
            lT = me.GetCp() - cchNew + cchOld;
            while (rpOld.IsValid() && pled->_cpMatchOld < lT)
            {
                if(rpOld->_fForceNewLine)
                    pled->_yMatchOld += rpOld->_yHeight;
                pled->_cpMatchOld += rpOld->_cch;

                BOOL fDone=FALSE;
                BOOL fSuccess = TRUE;
                while (!fDone)
                {
                    if( !rpOld.NextLine(FALSE,FALSE) )     // NextRun()
                    {
                        // No more line array entries so we can give up on
                        // trying to match for good.
                        fTryForMatch = FALSE;
                        frpOldValid = FALSE;
                        fDone = TRUE;
                        fSuccess = FALSE;
                    }
                    if (!rpOld->IsFrame() ||
                            (rpOld->IsFrame() && rpOld->_fFrameBeforeText))
                        fDone = TRUE;
                }
                if (!fSuccess)
                    break;
            }
        }

        // skip frame in old line array
        if (rpOld->IsFrame() && !rpOld->_fFrameBeforeText)
        {
            BOOL fDone=FALSE;
            while (!fDone)
            {
                if (!rpOld.NextLine(FALSE,FALSE))
                {
                    // No more line array entries so we can give up on
                    // trying to match for good.
                    fTryForMatch = FALSE;
                    frpOldValid = FALSE;
                    fDone = TRUE;
                }
                if (!rpOld->IsFrame())
                    fDone = TRUE;
            }
        }

        pliNew = aryNewLines.Count() > 0 ? aryNewLines.Elem(aryNewLines.Count() - 1) : NULL;

        // If perfect match, stop
        if(   frpOldValid
           && rpOld.IsValid()
           && pled->_cpMatchOld == lT
           && rpOld->_cch != 0
           && (   !rpOld->_fPartOfRelChunk  // lines do not match if they are a part of
               || rpOld->_fFirstFragInLine  // the previous relchunk (bug 48513 SujalP)
              )
           && pliNew
           && pliNew->_fForceNewLine
           && (yHeight - pliNew->_yHeight > RecalcLinePtr._marginInfo._yBottomLeftMargin)
           && (yHeight - pliNew->_yHeight > RecalcLinePtr._marginInfo._yBottomRightMargin)
          )
        {
            BOOL fFoundMatch = TRUE;

            if(rpOld->_xLeftMargin || rpOld->_xRightMargin)
            {
                // we might have found a match based on cp, but if an
                // aligned site is resized to a smaller size. There might
                // be lines that used to be aligned to the aligned site
                // which are not longer aligned to it. If so, recalc those lines.
                RecalcLinePtr.RecalcMargins(iOldLine, iOldLine + aryNewLines.Count(), yHeight,
                                                rpOld->_yBeforeSpace);
                if(RecalcLinePtr._marginInfo._xLeftMargin != rpOld->_xLeftMargin ||
                    (rpOld->_xLeftMargin + rpOld->_xLineWidth +
                        RecalcLinePtr._marginInfo._xRightMargin) < pFlowLayout->GetMaxLineWidth())
                {
                    fFoundMatch = FALSE;
                }
            }

            // There are cases where we've matched characters, but want to continue
            // to recalc anyways. This requires us to instantiate a new measurer.
            if (fFoundMatch && !fChangedLineSpacing && rpOld < LineCount() &&
                (rpOld->_fFirstInPara || pliNew->_fHasEOP))
            {
                BOOL                  fInner;
                const CParaFormat *   pPF;
                CLSMeasurer           tme(this, me.GetCp(), pci);

                if (!tme._pLS)
                    goto err;

                if(pliNew->_fHasEOP)
                {
                    rpOld->_fFirstInPara = TRUE;
                }
                else
                {
                    rpOld->_fFirstInPara = FALSE;
                    rpOld->_fHasBulletOrNum = FALSE;
                }

                // If we've got a <DL COMPACT> <DT> line. For now just check to see if
                // we're under the DL.

                pPF = tme.CurrBranch()->GetParaFormat();

                fInner = SameScope(tme.CurrBranch(), pElementContent);

                if (pPF->HasCompactDL(fInner))
                {
                    // If the line is a DT and it's the end of the paragraph, and the COMPACT
                    // attribute is set.
                    fChangedLineSpacing = TRUE;
                }

                // Check to see if the line height is the same. This is necessary
                // because there are a number of block operations that can
                // change the prespacing of the next line.
                else
                {
                    // We'd better not be looking at a frame here.
                    Assert (!rpOld->IsFrame());

                    // Make it possible to check the line space of the
                    // current line.
                    tme.InitLineSpace (&me, rpOld);

                    RecalcLinePtr.CalcInterParaSpace (&tme,
                            pled->_iliFirst + aryNewLines.Count() - 1, FALSE);

                    if (rpOld->_yBeforeSpace != tme._li._yBeforeSpace ||
                        rpOld->_fHasBulletOrNum != tme._li._fHasBulletOrNum)
                    {
                        rpOld->_fHasBulletOrNum = tme._li._fHasBulletOrNum;

                        fChangedLineSpacing = TRUE;
                    }
                }
            }
            else
                fChangedLineSpacing = FALSE;


            if (fFoundMatch && !fChangedLineSpacing)
            {
                PerfDbgLog1(tagRecalc, this, "Found match with old line #%d", rpOld.GetLineIndex());
                pled->_iliMatchOld = rpOld;

                // Replace old lines by new ones
                lT = rpOld - pled->_iliFirst;
                rpOld = pled->_iliFirst;

                fReplaceResult = rpOld.Replace(lT, &aryNewLines);
                if (!fReplaceResult)
                {
                    Assert(FALSE);
                    goto err;
                }

                frpOldValid =
                    rpOld.SetRun( rpOld.GetIRun() + aryNewLines.Count(), 0 );

                aryNewLines.Clear(AF_DELETEMEM);           // Clear aux array
                iOldLine = rpOld;

                // Remember information about match after editing
                Assert((cp = rpOld.GetCp() + cpLayoutFirst) == (LONG) me.GetCp());
                pled->_yMatchNew = yHeight;
                pled->_iliMatchNew = rpOld;
                pled->_cpMatchNew = me.GetCp();

                _dcpCalcMax = me.GetCp() - cpLayoutFirst;

                // Compute height and cp after all matches
                if( frpOldValid && rpOld.IsValid() )
                {
                    do
                    {
                        if(rpOld->_fForceNewLine)
                        {
                            yHeight += rpOld->_yHeight;
                            // fix for bug 16519
                            // Keep track of the line that contributes to the max height, ex: if the
                            // last lines top margin is negative.
                            if(yHeightDisplay < yHeight)
                                yHeightDisplay = yHeight;
                        }
                        else if(rpOld->IsFrame())
                        {
                            yAlignDescent = yHeight + rpOld->_yHeight;
                        }


                        _dcpCalcMax += rpOld->_cch;
                    }
                    while( rpOld.NextLine(FALSE,FALSE) ); // NextRun()
                }

                // Make sure _dcpCalcMax is sane after the above update
                AssertSz(GetMaxCpCalced() <= cpLast,
                    "CDisplay::RecalcLines match extends beyond EOF");

                // We stop calculating here.Note that if _dcpCalcMax < size
                // of text, this means a background recalc is in progress.
                // We will let that background recalc get the arrays
                // fully in sync.
                AssertSz(GetMaxCpCalced() <= cpLast,
                        "CDisplay::Match less but no background recalc");

                if (GetMaxCpCalced() != cpLast)
                {
                    // This is going to be finished by the background recalc
                    // so set the done flag appropriately.
                    fDone = FALSE;
                }

                goto match;
            }
        }

        // Add a new line
        pliNew = RecalcLinePtr.AddLine();
        if (!pliNew)
        {
            Assert(FALSE);
            goto err;
        }

        PerfDbgLog1(tagRecalc, this, "Measuring new line from cp = %d", me.GetCp());

        // measure can add lines for aligned sites
        iNewLine = iOldLine + aryNewLines.Count() - 1;

        uiFlags = MEASURE_BREAKATWORD |
                    (yHeight == 0 ? MEASURE_FIRSTLINE : 0) |
                    (fFirstInPara ? MEASURE_FIRSTINPARA : 0);

        if ( GetWordWrap() && GetWrapLongLines() )
        {
            uiFlags |= MEASURE_BREAKLONGLINES;
        }

        if (long(me.GetCp()) == pFlowLayout->GetContentLastCp() - 1)
        {
            uiFlags |= MEASURE_EMPTYLASTLINE;
        }

        //
        // Cache the previous dispnode to insert a new content node
        // if the next line measured has negative margins(offset) which
        // causes the next line on top of the previous lines.
        //
        pDNBefore = me._pDispNodePrev;
        iLinePrev = iNewLine;
        yHeightPrev = yHeight;

        if (!RecalcLinePtr.MeasureLine(me, uiFlags,
                                 &iNewLine, &yHeight, &yAlignDescent,
                                 NULL, NULL))
        {
            goto err;
        }

        // fix for bug 16519
        // Keep track of the line that contributes to the max height, ex: if the
        // last lines top margin is negative.
        if(yHeightDisplay < yHeight)
            yHeightDisplay = yHeight;

        //
        // iNewLine returned is the last text line inserted. There ay be
        // aligned lines and clear lines added before and after the
        // text line
        //
        pliNew = iNewLine >=0 ? RecalcLinePtr[iNewLine] : NULL;

        fFirstInPara = (iNewLine >= 0)
                            ? pliNew->IsNextLineFirstInPara()
                            : TRUE;
        if (iNewLine >= 0)
        {
            HandleNegativelyPositionedContent(pliNew, &me, pDNBefore, iLinePrev, yHeightPrev);
        }

        --iMinimumLinesToRecalc;
        if(iMinimumLinesToRecalc < 0 &&
           fBackground &&  (GetTickCount() >= (DWORD)BgndTickMax())) // GetQueueStatus(QS_ALLEVENTS))
        {
            fDone = FALSE;          // Took too long, stop for now
            break;
        }

        if (    fAllowBackgroundRecalc
            &&  yHeight > YWait()
            &&  (LONG) me.GetCp() > CPWait()
            &&  cliWait-- <= 0)
        {                           // Not really done, just past region we're
            fDone = FALSE;          //  waiting for so let background recalc
            break;                  //  take it from here
        }
    }                               // while(me < cpLast ...

no_match:
    // Didn't find match: whole line array from _iliFirst needs to be changed
    // Indicate no match
    pled->SetNoMatch();

    // Update old match position with previous total height of the document so
    // that UpdateView can decide whether to invalidate past the end of the
    // document or not
    pled->_iliMatchOld = LineCount();
    pled->_cpMatchOld  = cpLast + cchOld - cchNew;
    pled->_yMatchOld   = _yHeightMax;

    // Update last recalced cp
    _dcpCalcMax = me.GetCp() - cpLayoutFirst;

    // Replace old lines by new ones
    rpOld = pled->_iliFirst;

    // We store the result from the replace because although it can fail the
    // fields used for first visible must be set to something sensible whether
    // the replace fails or not. Further, the setting up of the first visible
    // fields must happen after the Replace because the lines could have
    // changed in length which in turns means that the first visible position
    // has failed.
    fReplaceResult = rpOld.Replace(-1, &aryNewLines);
    if (!fReplaceResult)
    {
        Assert(FALSE);
        goto err;
    }

    // Adjust first affected line if this line is gone
    // after replacing by new lines

    if(pled->_iliFirst >= LineCount() && LineCount() > 0)
    {
        Assert(pled->_iliFirst == LineCount());
        pled->_iliFirst = LineCount() - 1;
        Assert(!Elem(pled->_iliFirst)->IsFrame());
        pled->_yFirst -= Elem(pled->_iliFirst)->_yHeight;

        //
        // See comment before as to why its legal for this to be possible
        //
        //AssertSz(pled->_yFirst >= 0, "CDisplay::RecalcLines _yFirst < 0");
        pled->_cpFirst -= Elem(pled->_iliFirst)->_cch;
    }

match:

    // If there is a background on the paragraph, we have to make sure to redraw the
    // lines to the end of the paragraph.
    for (;pled->_iliMatchNew < LineCount();)
    {
        pliNew = Elem(pled->_iliMatchNew);
        if (pliNew)
        {
            if (!pliNew->_fHasBackground)
                break;

            pled->_iliMatchOld++;
            pled->_iliMatchNew++;
            pled->_cpMatchOld += pliNew->_cch;
            pled->_cpMatchNew += pliNew->_cch;
            me.Advance(pliNew->_cch);
            if (pliNew->_fForceNewLine)
            {
                pled->_yMatchOld +=  pliNew->_yHeight;
                pled->_yMatchNew +=  pliNew->_yHeight;
            }
            if (pliNew->_fHasEOP)
                break;
        }
        else
            break;
    }

    _fRecalcDone = fDone;
    _yCalcMax = yHeightDisplay;

    PerfDbgLog1(tagRecalc, this, "CDisplay::RecalcLine(tpFirst, ...) - Done. Recalced down to line #%d", LineCount() - 1);

    if (HasBgRecalcInfo())
    {
        CBgRecalcInfo * pBgRecalcInfo = BgRecalcInfo();
        // Clear wait fields since we want caller's to set them up.
        pBgRecalcInfo->_yWait = -1;
        pBgRecalcInfo->_cpWait = -1;
    }

    // We've measured the last line in the document. Do we want an empty lne?
    if ((LONG)me.GetCp() == cpLast)
    {
        LONG ili = LineCount() - 1;
        long lPadding[PADDING_MAX];

        //
        // Update the padding once we measure the last line
        //
        GetPadding(pci, lPadding);
        _yBottomMargin = lPadding[PADDING_BOTTOM];

        // If we haven't measured any lines (deleted an empty paragraph),
        // we need to get a pointer to the last line rather than using the
        // last line measured.
        while (ili >= 0)
        {
            pliNew = Elem(ili);
            if(pliNew->IsTextLine())
                break;
            else
                ili--;
        }

        // If the last line has a paragraph break or we don't have any
        // line any more, we need to create a empty line only if we are in design mode
        if (    LineCount() == 0
            ||  (   GetFlowLayout()->IsEditable()
                &&  pliNew
                &&  (   pliNew->_fHasEOP
                    ||  pliNew->_fHasBreak
                    ||  pliNew->_fSingleSite)))
        {
            // Only create an empty line after a table (at the end
            // of the document) if the table is completely loaded.
            if (pliNew == NULL ||
                !pliNew->_fSingleSite ||
                me._pRunOwner->Tag() != ETAG_TABLE ||
                DYNCAST(CTableLayout, me._pRunOwner)->IsCompleted())
            {
                RecalcLinePtr.Init((CLineArray *)this, 0, NULL);
                CreateEmptyLine(&me, &RecalcLinePtr, &yHeight,
                                pliNew ? pliNew->_fHasEOP : TRUE);
                // fix for bug 16519
                // Keep track of the line that contributes to the max height, ex: if the
                // last lines top margin is negative.
                if(yHeightDisplay < yHeight)
                    yHeightDisplay = yHeight;
            }
        }

        // In table cells, Netscape actually adds the interparagraph spacing
        // for any closed tags to the height of the table cell.
        // BUGBUG: This actually applies the spacing to all nested text containers, not just
        //         table cells. Is this correct? (brendand)
        // It's not correct to add the spacing at all, but given that Netscape
        // has done so, and that they will probably do so for floating block
        // elements. Arye.
        else
        {
            int iLineLast = ili;
            
            // we need to force scrolling when bottom-margin is set on the last block tag
            // in body. (bug 20400)

            // Only do this if we're the last line in the text site.
            // This means that the last line is a text line.
            if (iLineLast == LineCount() - 1)
            {
                while (iLineLast-- > 0 &&   // Lines available
                       !pliNew->_fForceNewLine)   // Just a dummy line.
                    --pliNew;
            }
            if (iLineLast > 0 || pliNew->_fForceNewLine)
            {
                _yBottomMargin += RecalcLinePtr.NetscapeBottomMargin(&me);
            }
        }
    }

    if (fDone)
    {
        RecalcMost();

        if(fBackground)
        {
            StopBackgroundRecalc();
        }
    }

    xWidth = CalcDisplayWidth();
    yHeightMax = max(yHeightDisplay, yAlignDescent);

    Assert (pled);

    // If the size changed, re-size or update scrollbars as appropriate
    //
    // Fire a resize notification if the content size changed
    //  
    // fix for 52305, change to margin bottom affects the content size,
    // so we should fire a resize.
    //
    if (    yHeightMax + yBottomMarginOld != _yHeightMax + _yBottomMargin
        ||  yHeightDisplay != _yHeight
        ||  xWidth != _xWidth)
    {
        _xWidth       = xWidth;
        _yHeight      = yHeightDisplay;
        _yHeightMax   = yHeightMax;

        pFlowLayout->SizeContentDispNode(CSize(GetMaxWidth(), GetHeight()));

        // If our contents affects our size, ask our parent to initiate a re-size
        ElementResize(pFlowLayout);
    }
    else if (_fHasMultipleTextNodes)
    {
        pFlowLayout->SizeContentDispNode(CSize(GetMaxWidth(), GetHeight()));
    }

    // Update the relative line cache.
    if (pFlowLayout->_fContainsRelative)
        UpdateRelDispNodeCache(pled);

    AdjustDispNodes(me._pDispNodePrev, pled);

    pFlowLayout->NotifyMeasuredRange(pled->_cpFirst, me.GetCp());

    if (pled->_cpMatchNew != MAXLONG && (pled->_yMatchNew - pled->_yMatchOld))
    {
        CSize size(0, pled->_yMatchNew - pled->_yMatchOld);

        pFlowLayout->NotifyTranslatedRange(size, pled->_cpMatchNew, cpLayoutFirst + _dcpCalcMax);
    }

    // If not done, do the rest in background
    if(!fDone && !fBackground)
        StartBackgroundRecalc(pci->_grfLayout);

    if(fDone)
    {
        CheckLineArray();
        _fLineRecalcErr = FALSE;
    }

cleanup:

    SwitchesEndTimer(SWITCHES_TIMER_RECALCLINES);

    return TRUE;

err:
    if(!_fLineRecalcErr)
    {
        _dcpCalcMax = me.GetCp() - cpLayoutFirst;
        _yCalcMax   = yHeightDisplay;
        _fLineRecalcErr = FALSE;            //  fix up CArray & bail
    }

    TraceTag((tagError, "CDisplay::RecalcLines() failed"));

    pled->SetNoMatch();

    if(!fReplaceResult)
    {
        FlushRecalc();
    }

    // Update the relative line cache.
    if (pFlowLayout->_fContainsRelative)
        UpdateRelDispNodeCache(pled);

    AdjustDispNodes(me._pDispNodePrev, pled);

    return FALSE;
}

/*
 *  CDisplay::UpdateView(&tpFirst, cchOld, cchNew, fRecalc)
 *
 *  @mfunc
 *      Recalc lines and update the visible part of the display
 *      (the "view") on the screen.
 *
 *  @devnote
 *      --- Use when in-place active only ---
 *
 *  @rdesc
 *      TRUE if success
 */
BOOL CDisplay::UpdateView(
    CCalcInfo * pci,
    LONG cpFirst,   //@parm Text ptr where change happened
    LONG cchOld,    //@parm Count of chars deleted
    LONG cchNew)   //@parm No recalc need (only rendering change) = FALSE
{
    BOOL            fReturn = TRUE;
    BOOL            fNeedViewChange = FALSE;
    RECT            rcView;
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    CRect           rc;
    long            xWidthParent;
    long            yHeightParent;

    BOOL fInvalAll = FALSE;
    CStackDataAry < RECT, 10 > aryInvalRects(Mt(CDisplayUpdateView_aryInvalRects_pv));

    Assert(pci);

    if (_fNoUpdateView)
        return fReturn;

    // If we have no width, don't even try to recalc, it causes
    // crashes in the scrollbar code, and is just a waste of
    // time, anyway.
    // However, since we're not sized, request sizing from the parent
    // of the ped (this will leave the necessary bread-crumb chain
    // to ensure we get sized later) - without this, not all containers
    // of text sites (e.g., 2D sites) will know to size the ped.
    // (This is easier than attempting to propagate back out that sizing
    //  did not occur.)

    if (    GetViewWidth() <= 0
        && !pFlowLayout->_fContentsAffectSize)
    {
        FlushRecalc();
        return TRUE;
    }


    pFlowLayout->GetClientRect((CRect *)&rcView);

    GetViewWidthAndHeightForChild(pci, &xWidthParent, &yHeightParent);

    //
    // BUGBUG(SujalP, SriniB and BrendanD): These 2 lines are really needed here
    // and in all places where we instantiate a CI. However, its expensive right
    // now to add these lines at all the places and hence we are removing them
    // from here right now. We have also removed them from CTxtSite::CalcTextSize()
    // for the same reason. (Bug#s: 58809, 62517, 62977)
    //

    pci->SizeToParent(xWidthParent, yHeightParent);

    // If we get here, we are updating some general characteristic of the
    // display and so we want the cursor updated as well as the general
    // change otherwise the cursor will land up in the wrong place.
    _fUpdateCaret = TRUE;

    if (!LineCount())
    {
        // No line yet, recalc everything
        RecalcView(pci, TRUE);

        // Invalidate entire view rect
        fInvalAll = TRUE;
        fNeedViewChange = TRUE;
    }
    else
    {
        CLed            led;

        if( cpFirst > GetMaxCpCalced())
        {
            // we haven't even calc'ed this far, so don't bother with updating
            // here.  Background recalc will eventually catch up to us.
            return TRUE;
        }

        AssertSz(cpFirst <= GetMaxCpCalced(),
                 "CDisplay::UpdateView(...) - cpFirst > GetMaxCpCalced()");

        if (!RecalcLines(pci, cpFirst, cchOld, cchNew, FALSE, &led))
        {
            // we're in deep crap now, the recalc failed
            // let's try to get out of this with our head still mostly attached
            fReturn = FALSE;
            fInvalAll = TRUE;
            fNeedViewChange = TRUE;
        }

        if (!fInvalAll)
        {
            // Invalidate all the individual rectangles necessary to work around
            // any embedded images. Also, remember if this was a simple or a complex
            // operation so that we can avoid scrolling in difficult places.
            CLine * pLine;
            int     iLine, iLineLast;
            long    xLineLeft, xLineRight, yLineTop, yLineBottom;
            long    yTop;
            long    dy = led._yMatchNew - led._yMatchOld;

            // Start out with a zero area rectangle.
            // NOTE(SujalP): _yFirst can legally be less than 0. Its OK, because
            // the rect we are constructing here is going to be used for inval purposes
            // only and that is going to clip it to the content rect of the flowlayout.
            yTop      =
            rc.bottom =
            rc.top    = led._yFirst;
            rc.left   = MAXLONG;
            rc.right  = 0;

            // Need this to adjust for negative line heights.
            // Note that this also initializes the counter for the
            // for loop just below.
            iLine = led._iliFirst;

            // Accumulate rectangles of lines and invalidate them.
            iLineLast = min(led._iliMatchNew, LineCount());

            // Invalidate only the lines that have been touched by the recalc
            for (; iLine < iLineLast; iLine++)
            {
                pLine = Elem(iLine);

                if (pLine == NULL)
                    break;

                // Get the left and right edges of the line.
                if(!_fRTL)
                {
                    xLineLeft  = pLine->_xLeftMargin;
                    xLineRight = pLine->_xLeftMargin + pLine->_xLineWidth;
                }
                else
                {
                    xLineLeft  = - (pLine->_xRightMargin + pLine->_xLineWidth);
                    xLineRight = - (pLine->_xRightMargin);
                }

                // Get the top and bottom edges of the line
                yLineTop    = yTop + pLine->GetYLineTop();
                yLineBottom = yTop + pLine->GetYLineBottom();

                // First line of a new rectangle
                if (rc.right < rc.left)
                {
                    rc.left  = xLineLeft;
                    rc.right = xLineRight;
                }

                // Different margins, output the old one and restart.
                else if (rc.left != xLineLeft || rc.right != xLineRight)
                {
                    // if we have multiple chunks in the same line
                    if( rc.right  == xLineLeft &&
                        rc.top    == yLineTop  &&
                        rc.bottom == yLineBottom )
                    {
                        rc.right = xLineRight;
                    }
                    else
                    {
                        IGNORE_HR(aryInvalRects.AppendIndirect(&rc));

                        fNeedViewChange = TRUE;

                        // Zero height.
                        rc.top    =
                        rc.bottom = yTop;

                        // Just the width of the given line.
                        rc.left  = xLineLeft;
                        rc.right = xLineRight;
                    }
                }

                // Negative line height.
                rc.top = min(rc.top, yLineTop);

                rc.bottom = max(rc.bottom, yLineBottom);

                // Otherwise just accumulate the height.
                if(pLine->_fForceNewLine)
                {
                    yTop  += pLine->_yHeight;

                    // Stop when reaching the bottom of the visible area
                    if (rc.bottom > rcView.bottom)
                        break;
                }
            }

// BUBUG (srinib) - This is a temporary hack to handle the
// scrolling case until the display tree provides the functionality
// to scroll an rc in view. If the new content height changed then
// scroll the content below the change by the dy. For now we are just
// to the end of the view.
            if(dy)
            {
                rc.left   = rcView.left;
                rc.right  = rcView.right;
                rc.bottom = rcView.bottom;
            }

            // Get the last one.
            if (rc.right > rc.left && rc.bottom > rc.top)
            {
                IGNORE_HR(aryInvalRects.AppendIndirect(&rc));
                fNeedViewChange = TRUE;
            }

            // There might be more stuff which has to be
            // invalidated because of lists (numbers might change etc)
            if (UpdateViewForLists(&rcView,   cpFirst,
                                   iLineLast, rc.bottom, &rc))
            {
                IGNORE_HR(aryInvalRects.AppendIndirect(&rc));
                fNeedViewChange = TRUE;
            }

            if (    led._yFirst >= rcView.top
                &&  (   led._yMatchNew <= rcView.bottom
                    ||  led._yMatchOld <= rcView.bottom))
            {
                WaitForRecalcView(pci);
            }
        }
    }

    {
        // Now do the invals
        if (fInvalAll)
        {
            pFlowLayout->Invalidate();
        }
        else
        {
            pFlowLayout->Invalidate(&aryInvalRects[0], aryInvalRects.Size());
        }
    }

    if (fNeedViewChange)
    {
        pFlowLayout->ViewChange(FALSE);
    }

    CheckView();

#ifdef DBCS
    UpdateIMEWindow();
#endif

    return fReturn;
}

/*
 *  CDisplay::CalcDisplayWidth()
 *
 *  @mfunc
 *      Computes and returns width of this display by walking line
 *      CArray and returning the widest line.  Used for
 *      horizontal scroll bar routines
 *
 *  @todo
 *      This should be done by a background thread
 */

LONG CDisplay::CalcDisplayWidth ()
{
    LONG    ili = LineCount();
    CLine * pli;
    LONG    xWidth = 0;

    if(ili)
    {
        // Note: pli++ breaks array encapsulation (pli = &(*this)[ili] doesn't)
        pli = Elem(0);
        for(xWidth = 0; ili--; pli++)
        {
            // calc line width based on the direction
            if(!_fRTL)
            {
                xWidth = max(xWidth, pli->GetTextRight(ili == 0) + pli->_xRight);
            }
            else
            {
                xWidth = max(xWidth, pli->GetRTLTextLeft() + pli->_xLeft);
            }
        }
    }

    return xWidth;
}


/*
 *  CDisplay::StartBackgroundRecalc()
 *
 *  @mfunc
 *      Starts background line recalc (at _dcpCalcMax position)
 *
 *  @todo
 *      ??? CF - Should use an idle thread
 */
void CDisplay::StartBackgroundRecalc(DWORD grfLayout)
{
    // We better be in the middle of the job here.
    Assert (LineCount() > 0);

    Assert(CanHaveBgRecalcInfo());

    // StopBack.. kills the recalc task, *if it exists*
    StopBackgroundRecalc () ;

    EnsureBgRecalcInfo();

    if(!RecalcTask() && GetMaxCpCalced() < GetLastCp())
    {
        BgRecalcInfo()->_pRecalcTask = new CRecalcTask (this, grfLayout) ;
        if (RecalcTask())
        {
            _fRecalcDone = FALSE;
        }
        // BUGBUG (sujalp): what to do if we fail on mem allocation?
        // One solution is to create this task as blocked when we
        // construct CDisplay, and then just unblock it here. In
        // StopBackgroundRecalc just block it. In CDisplay destructor,
        // destruct the CTask too.
    }
}


/*
 *  CDisplay::StepBackgroundRecalc()
 *
 *  @mfunc
 *      Steps background line recalc (at _dcpCalcMax position)
 *      Called by timer proc
 *
 *  @todo
 *      ??? CF - Should use an idle thread
 */
VOID CDisplay::StepBackgroundRecalc (DWORD dwTimeOut, DWORD grfLayout)
{
    LONG cch = GetLastCp() - (GetMaxCpCalced());

    // don't try recalc when processing OOM or had an error doing recalc or
    // if we are asserting.
    if(_fInBkgndRecalc || _fLineRecalcErr )
    {
#if DBG==1
        if(_fInBkgndRecalc)
            PerfDbgLog(tagRecalc, this, "avoiding reentrant background recalc");
        else
            PerfDbgLog(tagRecalc, this, "OOM: not stepping recalc");
#endif
        return;
    }

    _fInBkgndRecalc = TRUE;

    // Background recalc is over if no more characters or we are no longer
    // active.
    if(cch <= 0)
    {
        if (RecalcTask())
        {
            StopBackgroundRecalc();
        }

        CheckLineArray();

        goto Cleanup;
    }

    {
        CFlowLayout *   pFlowLayout = GetFlowLayout();
        CElement    *   pElementFL = pFlowLayout->ElementOwner();
        LONG            cp = GetMaxCpCalced();

        if (    !pElementFL->IsDisplayNone()
            ||   pElementFL->Tag() == ETAG_BODY)
        {
            CCalcInfo   CI;
            CLed        led;
            long        xParentWidth;
            long        yParentHeight;

            pFlowLayout->OpenView();

            // Setup the amount of time we have this time around
            Assert(BgRecalcInfo() && "Supposed to have a recalc info in stepbackgroundrecalc");
            BgRecalcInfo()->_dwBgndTickMax = dwTimeOut ;

            CI.Init(pFlowLayout);
            GetViewWidthAndHeightForChild(
                &CI,
                &xParentWidth,
                &yParentHeight,
                CI._smMode == SIZEMODE_MMWIDTH);
            CI.SizeToParent(xParentWidth, yParentHeight);

            CI._grfLayout = grfLayout;

            RecalcLines(&CI, cp, cch, cch, TRUE, &led);
        }
        else
        {
            CNotification  nf;

            // Kill background recalc, if the layout is hidden
            StopBackgroundRecalc();

            // BUGBUG (MohanB) Need to use ElementContent()?
            // calc the rest by accumulating a dirty range.
            nf.CharsResize(GetMaxCpCalced(), cch, pElementFL->GetFirstBranch());
            GetMarkup()->Notify(&nf);
        }
    }

Cleanup:
    _fInBkgndRecalc = FALSE;

    return;
}


/*
 *  CDisplay::StopBackgroundRecalc()
 *
 *  @mfunc
 *      Steps background line recalc (at _dcpCalcMax position)
 *      Called by timer proc
 *
 */
VOID CDisplay::StopBackgroundRecalc()
{
    if (HasBgRecalcInfo())
    {
        if (RecalcTask())
        {
            RecalcTask()->Terminate () ;
            RecalcTask()->Release () ;
            _fRecalcDone = TRUE;
        }
        DeleteBgRecalcInfo();
    }
}

/*
 *  CDisplay::WaitForRecalc(cpMax, yMax, pDI)
 *
 *  @mfunc
 *      Ensures that lines are recalced until a specific character
 *      position or ypos.
 *
 *  @rdesc
 *      success
 */
BOOL CDisplay::WaitForRecalc(
    LONG cpMax,     //@parm Position recalc up to (-1 to ignore)
    LONG yMax,      //@parm ypos to recalc up to (-1 to ignore)
    CCalcInfo * pci)

{
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    BOOL            fReturn = TRUE;
    LONG            cch;
    CCalcInfo       CI;

    Assert(cpMax < 0 || (cpMax >= GetFirstCp() && cpMax <= GetLastCp()));

    //
    //  Return immediately if hidden, already measured up to the correct point, or currently measuring
    //  or if there is no dispnode (ie haven't been CalcSize'd yet)

    if (    pFlowLayout->IsDisplayNone()
        &&  pFlowLayout->Tag() != ETAG_BODY)
        return fReturn;

    if ( !pFlowLayout->GetElementDispNode() )
        return FALSE;
    
    if (    yMax < 0
        &&  cpMax >= 0
        &&  (   (   !pFlowLayout->IsDirty()
                &&  cpMax <= GetMaxCpCalced())
            ||  (   pFlowLayout->IsDirty()
                &&  pFlowLayout->IsRangeBeforeDirty(cpMax - pFlowLayout->GetContentFirstCp(), 0))))
        return fReturn;

    if (    pFlowLayout->TestLock(CElement::ELEMENTLOCK_RECALC)
        ||  pFlowLayout->TestLock(CElement::ELEMENTLOCK_PROCESSREQUESTS))
        return FALSE;

    //
    //  Calculate up through the request location
    //

    if(!pci)
    {
        CI.Init(pFlowLayout);
        pci = &CI;
    }

    pFlowLayout->CommitChanges(pci);

    if (    (   yMax < 0
            ||  yMax >= _yCalcMax)
        &&  (   cpMax < 0
            ||  cpMax > GetMaxCpCalced()))
    {
        cch = GetLastCp() - GetMaxCpCalced();
        if(cch > 0 || LineCount() == 0)
        {

            HCURSOR     hcur = NULL;
            CDoc *      pDoc = pFlowLayout->Doc();

            if (EnsureBgRecalcInfo() == S_OK)
            {
                CBgRecalcInfo * pBgRecalcInfo = BgRecalcInfo();
                Assert(pBgRecalcInfo && "Should have a BgRecalcInfo");
                pBgRecalcInfo->_cpWait = cpMax;
                pBgRecalcInfo->_yWait  = yMax;
            }

            if (pDoc && pDoc->State() >= OS_INPLACE)
            {
                hcur = SetCursorIDC(IDC_WAIT);
            }
            TraceTag((tagWarning, "Lazy recalc"));

            if(GetMaxCpCalced() == GetFirstCp() )
            {
                fReturn = RecalcLines(pci);
            }
            else
            {
                CLed led;

                fReturn = RecalcLines(pci, GetMaxCpCalced(), cch, cch, FALSE, &led, TRUE);
            }

            // Either we were not waiting for a cp or if we were, then we have been calcd to that cp
            Assert(cpMax < 0 || GetMaxCpCalced() >= cpMax);

            SetCursor(hcur);
        }
    }

    return fReturn;
}

#ifdef WIN16
#pragma code_seg ("DISP2_TEXT")
#endif

/*
 *  CDisplay::WaitForRecalcIli
 *
 *  @mfunc
 *      Wait until line array is recalculated up to line <p ili>
 *
 *  @rdesc
 *      Returns TRUE if lines were recalc'd up to ili
 */


#pragma warning(disable:4702)   //  Ureachable code

BOOL CDisplay::WaitForRecalcIli (
    LONG ili,       //@parm Line index to recalculate line array up to
    CCalcInfo * pci)
{
    // BUGBUG for now (istvanc)
    return ili < LineCount();

    LONG cchGuess;

    while(!_fRecalcDone && ili >= LineCount())
    {
        cchGuess = 5 * (ili - LineCount() + 1) * Elem(0)->_cch;
        if(!WaitForRecalc(GetMaxCpCalced() + cchGuess, -1, pci))
            return FALSE;
    }
    return ili < LineCount();
}

//+----------------------------------------------------------------------------
//
//  Member:     WaitForRecalcView
//
//  Synopsis:   Calculate up through the bottom of the visible content
//
//  Arguments:  pci - CCalcInfo to use
//
//  Returns:    TRUE if all Ok, FALSE otherwise
//
//-----------------------------------------------------------------------------
BOOL
CDisplay::WaitForRecalcView(CCalcInfo * pci)
{
    return WaitForRecalc(-1, GetFlowLayout()->GetClientBottom(), pci);
}


#pragma warning(default:4702)   //  re-enable unreachable code

//===================================  View Updating  ===================================


/*
 *  CDisplay::SetViewSize(rcView)
 *
 *  Purpose:
 *      Set the view size and return whether a full recalc is needed
 *
 *  Returns:
 *      TRUE if a full recalc is needed
 */
BOOL CDisplay::SetViewSize(const RECT &rcView, BOOL fViewUpdate)
{
    BOOL    fRecalc = ((rcView.right - rcView.left) != _xWidthView);

    _xWidthView  = rcView.right  - rcView.left;
    _yHeightView = rcView.bottom - rcView.top;

    return fRecalc || !LineCount();
}

/*
 *  CDisplay::RecalcView
 *
 *  Sysnopsis:  Recalc view and update first visible line
 *
 *  Arguments:
 *      fFullRecalc - TRUE if recalc from first line needed, FALSE if enough
 *                    to start from _dcpCalcMax
 *
 *  Returns:
 *      TRUE if success
 */
BOOL CDisplay::RecalcView(CCalcInfo * pci, BOOL fFullRecalc)
{
    CFlowLayout * pFlowLayout = GetFlowLayout();
    BOOL          fAllowBackgroundRecalc;

    // If we have no width, don't even try to recalc, it causes
    // crashes in the scrollbar code, and is just a waste of time
    if (GetViewWidth() <= 0 && !pFlowLayout->_fContentsAffectSize)
    {
        FlushRecalc();
        return TRUE;
    }

    fAllowBackgroundRecalc = AllowBackgroundRecalc(pci);

    // If a full recalc (from first line) is not needed
    // go from current _dcpCalcMax on
    if (!fFullRecalc)
    {
        return (fAllowBackgroundRecalc
                        ? WaitForRecalcView(pci)
                        : WaitForRecalc(GetLastCp(), -1, pci));
    }

    // Else do full recalc
    BOOL  fRet = TRUE;

    // If all that the element is likely to have is a single line of plain text,
    // use a faster mechanism to compute the lines. This is great for perf
    // of <INPUT>
    if (pFlowLayout->Tag() == ETAG_INPUT)
    {
        fRet = RecalcPlainTextSingleLine(pci);
    }
    else
    {
        // full recalc lines
        if(!RecalcLines(pci))
        {
            // we're in deep crap now, the recalc failed
            // let's try to get out of this with our head still mostly attached
            fRet = FALSE;
            goto Done;
        }
    }
    CheckView();

Done:
    return fRet;
}

void
SetLineWidth(long xWidthMax, CLine * pli)
{
    pli->_xLineWidth = max(xWidthMax, pli->_xLeft + pli->_xWidth +
                                        pli->_xLineOverhang + pli->_xRight);
}

//+---------------------------------------------------------------
//
//  Member:     CDisplay::RecalcLineShift
//
//  Synopsis:   Run thru line array and adjust line shift
//
//---------------------------------------------------------------


void CDisplay::RecalcLineShift(CCalcInfo * pci, DWORD grfLayout)
{
    CFlowLayout * pFlowLayout = GetFlowLayout();
    LONG        lCount = LineCount();
    LONG        ili;
    LONG        iliFirstChunk = 0;
    BOOL        fChunks = FALSE;
    CLine *     pli;
    long        xShift;
    long        xWidthMax = GetMaxPixelWidth();

    Assert (pFlowLayout->_fSizeToContent ||
            (_fMinMaxCalced && !pFlowLayout->ContainsChildLayout()));

    for(ili = 0, pli = Elem(0); ili < lCount; ili++, pli++)
    {
        // if the current line does not force a new line, then
        // find a line that forces the new line and distribute
        // width.

        if(!fChunks && !pli->_fForceNewLine && !pli->IsFrame())
        {
            iliFirstChunk = ili;
            fChunks = TRUE;
        }

        if(pli->_fForceNewLine)
        {
            xShift = 0;

            if(!fChunks)
                iliFirstChunk = ili;
            else
                fChunks = FALSE;

            if(pli->_fJustified && long(pli->_fJustified) != JUSTIFY_FULL )
            {
                // for pre whitespace is already include in _xWidth
                xShift = xWidthMax - pli->_xLeftMargin - pli->_xLeft - pli->_xWidth - pli->_xLineOverhang -
                         pli->_xRight - pli->_xRightMargin - GetCaret();

                xShift = max(xShift, 0L);           // Don't allow alignment to go < 0
                                                    // Can happen with a target device
                if(long(pli->_fJustified) == JUSTIFY_CENTER)
                    xShift /= 2;
            }

            while(iliFirstChunk < ili)
            {
                pli = Elem(iliFirstChunk++);
                if(!_fRTL)
                    pli->_xLeft += xShift;
                else
                    pli->_xRight += xShift;

                pli->_xLineWidth = pli->_xLeft + pli->_xWidth +
                                    pli->_xLineOverhang + pli->_xRight;
            }

            pli = Elem(iliFirstChunk++);
            if(!_fRTL)
                pli->_xLeft += xShift;
            else
                pli->_xRight += xShift;

            SetLineWidth(xWidthMax, pli);
        }
    }

    if (pFlowLayout->_fContainsRelative)
    {
        VoidRelDispNodeCache();
        UpdateRelDispNodeCache(NULL);
    }

    //
    // Nested layouts need to be repositioned, to account for lineshift.
    //
    if (pFlowLayout->_fSizeToContent)
    {
        RecalcLineShiftForNestedLayouts();
    }

    //
    // NOTE(SujalP): Ideally I would like to do the NMR in all cases. However, that causes a misc perf
    // regession of about 2% on pages with a lot of small table cells. To avoid that problem I am doing
    // this only for edit mode. If you have other needs then add those cases to the if condition.
    // 
    if (pFlowLayout->IsEditable())
        pFlowLayout->NotifyMeasuredRange(GetFlowLayoutElement()->GetFirstCp(), GetMaxCpCalced());
}

void
CDisplay::RecalcLineShiftForNestedLayouts()
{
    CLayout     * pLayout;
    CFlowLayout * pFL = GetFlowLayout();
    CDispNode   * pDispNode = pFL->GetFirstContentDispNode();

    if (pFL->_fAutoBelow)
    {
        DWORD_PTR dw;

        for (pLayout = pFL->GetFirstLayout(&dw); pLayout; pLayout = pFL->GetNextLayout(&dw))
        {
            CElement * pElement = pLayout->ElementOwner();
            CTreeNode * pNode   = pElement->GetFirstBranch();
            const CFancyFormat * pFF = pNode->GetFancyFormat();

            if (    !pFF->IsAligned()
                &&  (   pFF->IsAutoPositioned()
                    ||  pNode->GetCharFormat()->IsRelative(FALSE)))
            {
                pElement->ZChangeElement();
            }
        }
        pFL->ClearLayoutIterator(dw);
    }

    pDispNode = pDispNode ? pDispNode->GetNextSiblingNode(TRUE) : NULL;
    if (pDispNode)
    {
        CLinePtr rp(this);

        do
        {
            // if the current disp node is not a text node
            if (pDispNode->GetDispClient() != pFL)
            {
                void * pvOwner;

                pDispNode->GetDispClient()->GetOwner(pDispNode, &pvOwner);

                if (pvOwner)
                {
                    CElement * pElement = DYNCAST(CElement, (CElement *)pvOwner);

                    //
                    // aligned layouts are not affected by text-align
                    //
                    if (!pElement->IsAligned())
                    {
                        htmlAlign  atAlign  = (htmlAlign)pElement->GetFirstBranch()->
                                                    GetParaFormat()->_bBlockAlign;

                        if (atAlign == htmlAlignRight || atAlign == htmlAlignCenter)
                        {
                            pLayout = pElement->GetCurLayout();

                            Assert(pLayout);

                            rp.RpSetCp(pElement->GetFirstCp(), FALSE, TRUE);

                            pLayout->SetPosition(
                                        rp->GetTextLeft() + pLayout->GetXProposed(),
                                        pLayout->GetPositionTop(), TRUE); // notify auto
                        }
                    }
                }
            }
            pDispNode = pDispNode->GetNextSiblingNode(TRUE);
        }
        while (pDispNode);
    }
}

/*
 *  CDisplay::CreateEmptyLine()
 *
 *  @mfunc
 *      Create an empty line
 *
 *  @rdesc
 *      TRUE - worked <nl>
 *      FALSE - failed
 *
 */
BOOL CDisplay::CreateEmptyLine(CLSMeasurer * pMe,
    CRecalcLinePtr * pRecalcLinePtr,
    LONG * pyHeight, BOOL fHasEOP )
{
    UINT uiFlags;
    LONG yAlignDescent;
    INT  iNewLine;

    // Make sure that this is being called appropriately
    AssertSz(!pMe || GetLastCp() == long(pMe->GetCp()),
        "CDisplay::CreateEmptyLine called inappropriately");

    // Assume failure
    BOOL    fResult = FALSE;

    // Add one new line
    CLine *pliNew = pRecalcLinePtr->AddLine();

    if (!pliNew)
    {
        Assert(FALSE);
        goto err;
    }

    iNewLine = pRecalcLinePtr->Count() - 1;

    Assert (iNewLine >= 0);

    uiFlags = fHasEOP ? MEASURE_BREAKATWORD |
                        MEASURE_FIRSTINPARA :
                        MEASURE_BREAKATWORD;

    uiFlags |= MEASURE_EMPTYLASTLINE;

    // If this is the first line in the document.
    if (*pyHeight == 0)
        uiFlags |= MEASURE_FIRSTLINE;

    if (!pRecalcLinePtr->MeasureLine(*pMe, uiFlags,
                                   &iNewLine, pyHeight, &yAlignDescent,
                                   NULL, NULL))
    {
        goto err;
    }

    // If we made it to here, everything worked.
    fResult = TRUE;

err:

    return fResult;
}


//====================================  Rendering  =======================================


//====================================  Rendering  =======================================


/*
 *  CDisplay::Render(rcView, rcRender)
 *
 *  @mfunc
 *      Searches paragraph boundaries around a range
 *
 *  returns: the lowest yPos
 */
void
CDisplay::Render (
    CFormDrawInfo * pDI,
    const RECT &rcView,     // View RECT
    const RECT &rcRender,   // RECT to render (must be container in
    CDispNode * pDispNode)
{
#ifdef SWITCHES_ENABLED
    if (IsSwitchNoRenderLines())
        return;
#endif

    CFlowLayout * pFlowLayout = GetFlowLayout();
    CElement    * pElementFL  = pFlowLayout->ElementOwner();
    LONG    ili;
    LONG    iliBgDrawn = -1;
    POINT   pt;
    LONG    cp;
    LONG    yLine;
    LONG    yLi = 0;
    long    lCount;
    CLine * pli = NULL;
    RECT    rcClip;
    BOOL    fLineIsPositioned;
    CRect   rcLocal;
    long    iliStart  = -1;
    long    iliFinish = -1;
    CPoint  ptOffset;
    CPoint* pptOffset = NULL;

#if DBG == 1
    LONG    cpLi;
#endif

    // BUGBUG: Assert removed for NT5 B3 (bug #75432)
    // IE6: Reenable for IE6+
    // AssertSz(!pFlowLayout->IsDirty(), "Rendering when layout is dirty -- not a measurer/renderer problem!");

    if (    !pFlowLayout->ElementOwner()->IsInMarkup()
        || (!pFlowLayout->IsEditable() && pFlowLayout->IsDisplayNone())
        ||   pFlowLayout->IsDirty())   // Prevent us from crashing when we do get here in retail mode.
    {
        return;
    }

    // Create renderer
    CLSRenderer lsre(this, pDI);

    if (!lsre._pLS)
        return;

    Assert(pDI->_rc == rcView);
    Assert(((CRect&)(pDI->_rcClip)) == rcRender);

    // Calculate line and cp to start the display at
    rcLocal = rcRender;
    rcLocal.OffsetRect(-((CRect&)rcView).TopLeft().AsSize());

    //
    // if the current layout has multiple text nodes then
    // compute the range of lines the belong to the disp node
    // being rendered.
    //
    if (_fHasMultipleTextNodes && pDispNode)
    {
        GetFlowLayout()->GetTextNodeRange(pDispNode, &iliStart, &iliFinish);

        Assert(iliStart < iliFinish);

        //
        // For backgrounds, RegionFromElement is going to return the
        // rects relative to the layout. So, when we have multiple text
        // nodes pass the (0, -top) as the offset to make the rects
        // text node relative
        //
        pptOffset    = &ptOffset;
        pptOffset->x = 0;
        pptOffset->y = -pDispNode->GetPosition().y;
    }

    //
    // For multiple text node, we want to search for the point only
    // in the lines owned by the text node.
    //
    ili = LineFromPos(rcLocal, &yLine, &cp, 0, iliStart, iliFinish);

    lCount = iliFinish < 0 ? LineCount() : iliFinish;

    if(lCount <= 0 || ili < 0)
        return;

    rcClip = rcRender;

    // Prepare renderer

    if(!lsre.StartRender(rcView, rcRender))
        return;

    // If we're just painting the inset, don't check all the lines.
    if (rcRender.right <= rcView.left ||
        rcRender.left >= rcView.right ||
        rcRender.bottom <= rcView.top ||
        rcRender.top >= rcView.bottom)
        return;

    // Calculate the point where the text will start being displayed
    pt = ((CRect &)rcView).TopLeft();
    pt.y += yLine;

    // Init renderer at the start of the first line to render
    lsre.SetCurPoint(pt);
    lsre.SetCp(cp, NULL);

    WHEN_DBG(cpLi = long(lsre.GetCp());)

    yLi = pt.y;

    // Check if this line begins BEFORE the previous line ended ...
    // Would happen with negative line heights:
    //
    //           ----------------------- <-----+---------- Line 1
    //                                         |
    //           ======================= <-----|-----+---- Line 2
    //  yBeforeSpace__________^                |     |
    //                        |                |     |
    //  yLi ---> -------------+--------- <-----+     |
    //                                               |
    //                                               |
    //           ======================= <-----------+
    //
    RecalcMost();

    // Render each line in the update rectangle

    for (; ili < lCount; ili++)
    {
        // current line
        pli = Elem(ili);
        
        // if the most negative line is out off the view from the current
        // yOffset, don't look any further, we have rendered all the lines
        // in the inval'ed region
        if (yLi + min(long(0), pli->GetYTop()) + _yMostNeg >= rcClip.bottom)
        {
            break;
        }

        fLineIsPositioned = FALSE;

        //
        // if the current line is interesting (ignore aligned, clear,
        // hidden and blank lines).
        //
        if(pli->_cch && !pli->_fHidden)
        {
            //
            // if the current line is relative get its y offset and
            // zIndex
            //
            if(pli->_fRelative)
            {
                fLineIsPositioned = TRUE;
            }
        }

        //
        // now check to see if the current line is in view
        //
        if( (yLi + min(long(0), pli->GetYTop())) > rcClip.bottom ||
            (yLi + pli->GetYLineBottom() < rcClip.top))
        {
            //
            // line is not in view, so skip it
            //
            lsre.SkipCurLine(pli);
        }
        else
        {
            //
            // current line is in view, so render it
            //
            // Note: we have to render the background on a relative line,
            // the parent of the relative element might have background.(srinib)
            // fix for #51465
            //
            // if the paragraph has background or borders then compute the bounding rect
            // for the paragraph and draw the background and/or border
            if(iliBgDrawn < ili &&
               (pli->_fHasParaBorder || // if we need to draw borders
                (pli->_fHasBackground && pElementFL->Doc()->PaintBackground())))
            {
                DrawBackgroundAndBorder(lsre.GetDrawInfo(), lsre.GetCp(), ili, lCount,
                                        &iliBgDrawn, yLi, &rcView, &rcClip, pptOffset);

                //
                // N.B. (johnv) Lines can be added by as
                // DrawBackgroundAndBorders (more precisely,
                // RegionFromElement, which it calls) waits for a
                // background recalc.  Recompute our cached line pointer.
                //
                pli = Elem(ili);
            }

            // if the current line has is positioned it will be taken care
            // of through the SiteDrawList and we shouldn't draw it here.
            //
            if (fLineIsPositioned)
            {
                lsre.SkipCurLine(pli);
            }
            else
            {
                //
                // Finally, render the current line
                //
                lsre.RenderLine(*pli);
            }
        }

        Assert(pli == Elem(ili));

        //
        // update the yOffset for the next line
        //
        if(pli->_fForceNewLine)
            yLi += pli->_yHeight;

        WHEN_DBG( cpLi += pli->_cch; )

        AssertSz( long(lsre.GetCp()) == cpLi,
                  "CDisplay::Render() - cp out of sync. with line table");
        AssertSz( lsre.GetCurPoint().y == yLi,
                  "CDisplay::Render() - y out of sync. with line table");

    }

    if (lsre._lastTextOutBy != CLSRenderer::DB_LINESERV)
    {
        lsre._lastTextOutBy = CLSRenderer::DB_NONE;
        SetTextAlign(lsre._hdc, TA_TOP | TA_LEFT);
    }
}

//+---------------------------------------------------------------------------
//
// Member:      DrawBackgroundAndBorders()
//
// Purpose:     Draw background and borders for elements on the current line,
//              and all the consecutive lines that have background.
//
//----------------------------------------------------------------------------
void
CDisplay::DrawBackgroundAndBorder(
            CFormDrawInfo * pDI,
            long            cpIn,
            LONG            ili,
            LONG            lCount,
            LONG          * piliDrawn,
            LONG            yLi,
            const RECT    * prcView,
            const RECT    * prcClip,
            const CPoint  * pptOffset)
{
    const CCharFormat  * pCF;
    const CFancyFormat * pFF;
    const CParaFormat  * pPF;
    CStackPtrAry < CTreeNode *, 8 > aryNodesWithBgOrBorder(Mt(CDisplayDrawBackgroundAndBorder_aryNodesWithBgOrBorder_pv));
    CDataAry <RECT> aryRects(Mt(CDisplayDrawBackgroundAndBorder_aryRects_pv));
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    CElement    *   pElementFL  = pFlowLayout->ElementContent();
    CMarkup     *   pMarkup = pFlowLayout->GetContentMarkup();
    BOOL            fPaintBackground = pElementFL->Doc()->PaintBackground();
    CTreeNode *     pNodeCurrBranch;
    CTreeNode *     pNode;
    CTreePos  *     ptp;
    long            ich;
    long            cpClip = cpIn;
    long            cp;
    long            lSize;


    // find the consecutive set of lines that have background
    while (ili < lCount && yLi + _yMostNeg < prcClip->bottom)
    {
        CLine * pli = Elem(ili++);

        // if the current line has borders or background then
        // continue otherwise return.
        if (!(pli->_fHasBackground && fPaintBackground) &&
            !pli->_fHasParaBorder)
        {
            break;
        }

        if (pli->_fForceNewLine)
        {
            yLi += pli->_yHeight;
        }

        cpClip += pli->_cch;
    }

    if(cpIn != cpClip)
        *piliDrawn = ili - 1;

    // initialize the tree pos that corresponds to the begin cp of the
    // current line
    ptp = pMarkup->TreePosAtCp(cpIn, &ich);

    cp = cpIn - ich;

    // first draw any backgrounds extending into the current region
    // from the previous line.

    pNodeCurrBranch = ptp->GetBranch();

    if(DifferentScope(pNodeCurrBranch, pElementFL))
    {
        pNode = pNodeCurrBranch;

        // run up the current branch and find the ancestors with background
        // or border
        while(pNode)
        {
            if(pNode->HasLayout())
            {
                CLayout * pLayout = pNode->GetCurLayout();

                if(pLayout == pFlowLayout)
                    break;

                Assert(pNode == pNodeCurrBranch);
            }
            else
            {
                // push this element on to the stack
                aryNodesWithBgOrBorder.Append(pNode);
            }
            pNode = pNode->Parent();
        }

        Assert(pNode);

        // now that we have all the elements with background or borders
        // for the current branch render them.
        for(lSize = aryNodesWithBgOrBorder.Size(); lSize > 0; lSize--)
        {
            CTreeNode * pNode = aryNodesWithBgOrBorder[lSize - 1];

            //
            // In design mode relative elements are drawn in flow (they are treated
            // as if they are not relative). Relative elements draw their own
            // background and their children's background. So, draw only the
            // background of ancestor's if any. (#25583)
            //
            if(pNode->IsRelative() )
            {
                pNodeCurrBranch = pNode;
                break;
            }
            else
            {
                pCF = pNode->GetCharFormat();
                pFF = pNode->GetFancyFormat();
                pPF = pNode->GetParaFormat();

                if (!pCF->IsVisibilityHidden() && !pCF->IsDisplayNone())
                {
                    BOOL fDrawBorder = pPF->_fPadBord  && pFF->_fBlockNess;
                    BOOL fDrawBackground = fPaintBackground &&
                                            (pFF->_lImgCtxCookie ||
                                             pFF->_ccvBackColor.IsDefined());

                    if (fDrawBackground || fDrawBorder)
                    {
                        DrawElemBgAndBorder(
                            pNode->Element(), &aryRects,
                            prcView, prcClip,
                            pDI, pptOffset,
                            fDrawBackground, fDrawBorder,
                            cpIn, -1);
                    }
                }
            }
        }


        //
        // In design mode relative elements are drawn in flow (they are treated
        // as if they are not relative).
        //
        if(pNodeCurrBranch->HasLayout() || pNode->IsRelative())
        {
            CTreePos * ptpBegin;

            pNodeCurrBranch->Element()->GetTreeExtent(&ptpBegin, &ptp);

            cp = ptp->GetCp();
        }

        cp += ptp->GetCch();
        ptp = ptp->NextTreePos();
    }


    // now draw the background of all the elements comming into scope of
    // in the cpRange
    while(ptp && cpClip >= cp)
    {
        if(ptp->IsBeginElementScope())
        {
            pNode = ptp->Branch();
            pCF   = pNode->GetCharFormat();

            // Background and border for a relative element or an element
            // with layout are drawn when the element is hit with a draw.
            if(pNode->HasLayout() || pCF->_fRelative)
            {
                if(DifferentScope(pNode, pElementFL))
                {
                    CTreePos * ptpBegin;

                    pNode->Element()->GetTreeExtent(&ptpBegin, &ptp);
                    cp = ptp->GetCp();
                }
            }
            else
            {
                pCF = pNode->GetCharFormat();
                pFF = pNode->GetFancyFormat();
                pPF = pNode->GetParaFormat();

                if (!pCF->IsVisibilityHidden() && !pCF->IsDisplayNone())
                {
                    BOOL fDrawBorder = pPF->_fPadBord  && pFF->_fBlockNess;
                    BOOL fDrawBackground = fPaintBackground &&
                                            (pFF->_lImgCtxCookie ||
                                             pFF->_ccvBackColor.IsDefined());

                    if (fDrawBackground || fDrawBorder)
                    {
                        DrawElemBgAndBorder(
                                pNode->Element(), &aryRects,
                                prcView, prcClip,
                                pDI, pptOffset,
                                fDrawBackground, fDrawBorder,
                                cp, -1);
                    }
                }
            }
        }

        cp += ptp->GetCch();
        ptp = ptp->NextTreePos();
    }
}

//+----------------------------------------------------------------------------
//
// Function:    BoundingRectForAnArrayOfRectsWithEmptyOnes
//
// Synopsis:    Find the bounding rect that contains a given set of rectangles
//              It does not ignore the rectangles that have left=right, top=bottom
//              or both. It still ignores the rects that have left=right=top=bottom=0
//
//-----------------------------------------------------------------------------

void
BoundingRectForAnArrayOfRectsWithEmptyOnes(RECT *prcBound, CDataAry<RECT> * paryRects)
{
    RECT *  prc;
    LONG    iRect;
    LONG    lSize = paryRects->Size();
    BOOL    fFirst = TRUE;

    SetRectEmpty(prcBound);

    for(iRect = 0, prc = *paryRects; iRect < lSize; iRect++, prc++)
    {
        if((prc->left <= prc->right && prc->top <= prc->bottom) &&
            (prc->left != 0 || prc->right != 0 || prc->top != 0 || prc->bottom != 0) )
        {
            if(fFirst)
            {
                *prcBound = *prc;
                fFirst = FALSE;
            }
            else
            {
                if(prcBound->left > prc->left) prcBound->left = prc->left;
                if(prcBound->top > prc->top) prcBound->top = prc->top;
                if(prcBound->right < prc->right) prcBound->right = prc->right;
                if(prcBound->bottom < prc->bottom) prcBound->bottom = prc->bottom;
            }
        }
    }
}


//+----------------------------------------------------------------------------
//
// Function:    BoundingRectForAnArrayOfRects
//
// Synopsis:    Find the bounding rect that contains a given set of rectangles
//
//-----------------------------------------------------------------------------

void
BoundingRectForAnArrayOfRects(RECT *prcBound, CDataAry<RECT> * paryRects)
{
    RECT *  prc;
    LONG    iRect;
    LONG    lSize = paryRects->Size();

    SetRectEmpty(prcBound);

    for(iRect = 0, prc = *paryRects; iRect < lSize; iRect++, prc++)
    {
        if(!IsRectEmpty(prc))
        {
            UnionRect(prcBound, prcBound, prc);
        }
    }
}


//+----------------------------------------------------------------------------
//
// Member:      DrawElementBackground
//
// Synopsis:    Draw the background for a an element, given the region it
//              occupies in the display
//
//-----------------------------------------------------------------------------

void
CDisplay::DrawElementBackground(CTreeNode * pNodeContext,
                                CDataAry <RECT> * paryRects, RECT * prcBound,
                                const RECT * prcView, const RECT * prcClip,
                                CFormDrawInfo * pDI)
{
    RECT    rcDraw;
    RECT    rcBound = { 0 };
    RECT *  prc;
    LONG    lSize;
    LONG    iRect;
    SIZE    sizeImg;
    RECT    rcBackClip;
    CPoint  ptBackOrg;
    BACKGROUNDINFO bginfo;

    const CFancyFormat * pFF = pNodeContext->GetFancyFormat();
    BOOL  fBlockElement = pNodeContext->Element()->IsBlockElement();

    Assert(pFF->_lImgCtxCookie || pFF->_ccvBackColor.IsDefined());

    CDoc *    pDoc    = GetFlowLayout()->Doc();
    CImgCtx * pImgCtx = (pFF->_lImgCtxCookie
                            ? pDoc->GetUrlImgCtx(pFF->_lImgCtxCookie)
                            : 0);

    if (pImgCtx && !(pImgCtx->GetState(FALSE, &sizeImg) & IMGLOAD_COMPLETE))
        pImgCtx = NULL;

    // if the background image is not loaded yet and there is no background color
    // return (we dont have anything to draw)
    if(!pImgCtx && !pFF->_ccvBackColor.IsDefined())
        return;

    // now given the rects for a given element
    // draw its background

    // if we have a background image, we need to compute its origin

    lSize = paryRects->Size();

    if(lSize == 0)
        return;

    memset(&bginfo, 0, sizeof(bginfo));

    bginfo.pImgCtx       = pImgCtx;
    bginfo.lImgCtxCookie = pFF->_lImgCtxCookie;
    bginfo.crTrans       = COLORREF_NONE;
    bginfo.crBack        = pFF->_ccvBackColor.IsDefined()
        ? pFF->_ccvBackColor.GetColorRef()
        : COLORREF_NONE;

    if (pImgCtx || fBlockElement)
    {
        if(!prcBound)
        {
            // compute the bounding rect for the element.
            BoundingRectForAnArrayOfRects(&rcBound, paryRects);
        }
        else
            rcBound = *prcBound;
    }

    if (pImgCtx)
    {
        if(!IsRectEmpty(&rcBound))
        {
            SIZE sizeBound;

            sizeBound.cx = rcBound.right - rcBound.left;
            sizeBound.cy = rcBound.bottom - rcBound.top;

            CalcBgImgRect(pNodeContext, pDI, &sizeBound, &sizeImg, &ptBackOrg, &rcBackClip);

            OffsetRect(&rcBackClip, rcBound.left, rcBound.top);

            ptBackOrg.x += rcBound.left - prcView->left;
            ptBackOrg.y += rcBound.top - prcView->top;

            bginfo.ptBackOrg = ptBackOrg;
            pDI->TransformToDeviceCoordinates(&bginfo.ptBackOrg);
        }
    }

    prc = *paryRects;
    rcDraw = *prc++;

    //
    // Background for block element needs to extend for the
    // left to right of rcBound.
    //
    if (fBlockElement)
    {
        rcDraw.left  = rcBound.left;
        rcDraw.right = rcBound.right;
    }

    for(iRect = 1; iRect <= lSize; iRect++, prc++)
    {
        if(iRect == lSize || !IsRectEmpty(prc))
        {
            if (iRect != lSize)
            {
                if (fBlockElement)
                {
                    if (prc->top < rcDraw.top)
                        rcDraw.top = prc->top;
                    if (prc->bottom > rcDraw.bottom)
                        rcDraw.bottom = prc->bottom;
                    continue;
                }
                else if (   prc->left == rcDraw.left
                        &&  prc->right == rcDraw.right
                        &&  prc->top == rcDraw.bottom)
                {
                    // add the current rect
                    rcDraw.bottom = prc->bottom;
                    continue;
                }
            }

            {
                IntersectRect(&rcDraw, prcClip, &rcDraw);

                if(!IsRectEmpty(&rcDraw))
                {
                    IntersectRect(&bginfo.rcImg, &rcBackClip, &rcDraw);
                    GetFlowLayout()->DrawBackground(pDI, &bginfo, &rcDraw);
                }

                if(iRect != lSize)
                    rcDraw = *prc;
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:    DrawElementBorder
//
// Synopsis:    Find the bounding rect that contains a given set of rectangles
//
//-----------------------------------------------------------------------------

void
CDisplay::DrawElementBorder(CTreeNode * pNodeContext,
                                CDataAry <RECT> * paryRects, RECT * prcBound,
                                const RECT * prcView, const RECT * prcClip,
                                CFormDrawInfo * pDI)
{
    CBorderInfo borderInfo;
    CElement *  pElement = pNodeContext->Element();

    if (pNodeContext->GetCharFormat()->IsVisibilityHidden())
        return;

    if ( !pElement->_fDefinitelyNoBorders &&
         FALSE == (pElement->_fDefinitelyNoBorders = !GetBorderInfoHelper(pNodeContext, pDI, &borderInfo, TRUE ) ) )
    {
        RECT rcBound;

        if(!prcBound)
        {
            if(paryRects->Size() == 0)
                return;

            // compute the bounding rect for the element.
            BoundingRectForAnArrayOfRects(&rcBound, paryRects);
        }
        else
            rcBound = *prcBound;

        DrawBorder(pDI, &rcBound, &borderInfo);
    }
}


// =============================  Misc  ===========================================

//+--------------------------------------------------------------------------------
//
// Synopsis: return true if this is the last text line in the line array
//---------------------------------------------------------------------------------
BOOL
CDisplay::IsLastTextLine(LONG ili)
{
    Assert(ili >= 0 && ili < LineCount());
    if (LineCount() == 0)
        return TRUE;
    
    for(LONG iliT = ili + 1; iliT < LineCount(); iliT++)
    {
        if(Elem(iliT)->IsTextLine())
            return FALSE;
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     FormattingNodeForLine
//
//  Purpose:    Returns the node which controls the formatting at the BOL. This
//              is needed because the first char on the line may not necessarily
//              be the first char in the paragraph.
//
//----------------------------------------------------------------------------
CTreeNode *
CDisplay::FormattingNodeForLine(
    LONG         cpForLine,                 // IN
    CTreePos    *ptp,                       // IN
    LONG         cchLine,                   // IN
    LONG        *pcch,                      // OUT
    CTreePos   **pptp,                      // OUT
    BOOL        *pfMeasureFromStart) const  // OUT
{
    CFlowLayout  *pFlowLayout = GetFlowLayout();
    BOOL          fIsEditable = pFlowLayout->IsEditable();
    CMarkup      *pMarkup     = pFlowLayout->GetContentMarkup();
    CTreeNode    *pNode       = NULL;
    CElement     *pElement;
    LONG          lNotNeeded;
    LONG          cch = cchLine;
    BOOL          fSawOpenLI  = FALSE;
    BOOL          fSeenAbsolute = FALSE;
    BOOL          fSeenBeginBlockTag = FALSE;

    // AssertSz(!pFlowLayout->IsDirty(), "Called when line array dirty -- not a measurer/renderer problem!");
    
    if (!ptp)
    {
        ptp = pMarkup->TreePosAtCp(cpForLine, &lNotNeeded);
        Assert(ptp);
    }
    else
    {
        Assert(ptp->GetCp() <= cpForLine);
        Assert(ptp->GetCp() + ptp->GetCch() >= cpForLine);
    }
    if (pfMeasureFromStart)
        *pfMeasureFromStart = FALSE;
    while(cch > 0)
    {
        if (ptp->IsPointer())
        {
            ptp = ptp->NextTreePos();
            continue;
        }

        if (ptp->IsText())
        {
            if (ptp->Cch())
                break;
            else
            {
                ptp = ptp->NextTreePos();
                continue;
            }
        }

        Assert(ptp->IsNode());

        if (fIsEditable && pfMeasureFromStart && ptp->IsNode() && ptp->ShowTreePos())
            *pfMeasureFromStart = TRUE;

        pNode = ptp->Branch();
        pElement = pNode->Element();
        
        if (ptp->IsBeginElementScope())
        {
            if (fSeenAbsolute)
                break;
            
            const CCharFormat *pCF = pNode->GetCharFormat();
            if (pCF->IsDisplayNone())
            {
                cch -= pFlowLayout->GetNestedElementCch(pElement, &ptp) - 1;
                pNode = pNode->Parent();
            }
            else if (pNode->NeedsLayout())
            {
                if (pNode->IsAbsolute())
                {
                    cch -= pFlowLayout->GetNestedElementCch(pElement, &ptp) - 1;
                    pNode = pNode->Parent();
                    fSeenAbsolute = TRUE;
                }
                else
                {
                    break;
                }
            }
            else if (pElement->Tag() == ETAG_BR)
            {
                break;
            }
            else if (   pElement->IsFlagAndBlock(TAGDESC_LIST)
                     && fSawOpenLI)
            {
                CTreePos * ptpT = ptp;

                do
                {
                    ptpT = ptpT->PreviousTreePos();
                } while (ptpT->GetCch() == 0);

                pNode = ptpT->Branch();

                break;
            }                        
            else if (pElement->IsTagAndBlock(ETAG_LI))
            {
                fSawOpenLI = TRUE;
            }
            else if (pFlowLayout->IsElementBlockInContext(pElement))
            {
                fSeenBeginBlockTag = TRUE;
            }
        }
        else if (ptp->IsEndNode())
        {
            if (fSeenAbsolute)
                break;
            
            //
            // If we encounter a break on empty block end tag, then we should
            // give vertical space otherwise a <X*></X> where X is a block element
            // will not produce any vertical space. (Bug 45291).
            //
            if (   fSeenBeginBlockTag 
                && pElement->_fBreakOnEmpty
                && pFlowLayout->IsElementBlockInContext(pElement)
               )
            {
                break;
            }
            
            if (   fSawOpenLI                       // Skip over the close LI, unless we saw an open LI,
                && ptp->IsEdgeScope()               // which would imply that we have an empty LI.  An
                && pElement->IsTagAndBlock(ETAG_LI) // empty LI gets a bullet, so we need to break.
               )
                break;
            pNode = pNode->Parent();
        }

        cch--;
        ptp = ptp->NextTreePos();
    }

    if (pcch)
    {
        *pcch = cchLine - cch;
    }

    if (pptp)
    {
        *pptp = ptp;
    }

    if (!pNode)
    {
        pNode = ptp->GetBranch();
        if (ptp->IsEndNode())
            pNode = pNode->Parent();
    }

    return pNode;
}

//+---------------------------------------------------------------------------
//
//  Member:     EndNodeForLine
//
//  Purpose:    Returns the first node which ends this line. If the line ends
//              because of insufficient width, then the node is the node
//              above the last character in the line, else it is the node
//              which causes the line to end (like /p).
//
//----------------------------------------------------------------------------
CTreeNode *
CDisplay::EndNodeForLine(
    LONG         cpEndForLine,              // IN
    CTreePos    *ptp,                       // IN
    LONG        *pcch,                      // OUT
    CTreePos   **pptp,                      // OUT
    CTreeNode  **ppNodeForAfterSpace) const // OUT
{
    CFlowLayout  *pFlowLayout = GetFlowLayout();
    BOOL          fIsEditable = pFlowLayout->IsEditable();
    CTreePos     *ptpStart, *ptpStop;
    CTreePos     *ptpNext = ptp;
    CTreePos     *ptpOriginal = ptp;
    CTreeNode    *pNode;
    CElement     *pElement;
    CTreeNode    *pNodeForAfterSpace = NULL;
    
    //
    // If we are in the middle of a text run then we do not need to
    // do anything, since this line will not be getting any para spacing
    //
    if (   ptpNext->IsText()
        && ptpNext->GetCp() < cpEndForLine
       )
        goto Cleanup;
    
    pFlowLayout->GetContentTreeExtent(&ptpStart, &ptpStop);

    //
    // We should never be here if we start measuring at the beginning
    // of the layout.
    //
    Assert(ptp != ptpStart);

    ptpStart = ptpStart->NextTreePos();
    while (ptp != ptpStart)
    {
        ptpNext = ptp;
        ptp = ptp->PreviousTreePos();

        if (ptp->IsPointer())
            continue;

        if (ptp->IsNode())
        {
            if (fIsEditable && ptp->ShowTreePos())
                break;

            pNode = ptp->Branch();
            pElement = pNode->Element();
            if (ptp->IsEndElementScope())
            {
                const CCharFormat *pCF = pNode->GetCharFormat();
                if (pCF->IsDisplayNone())
                {
                    pElement->GetTreeExtent(&ptp, NULL);
                }
                else if (pNode->NeedsLayout())
                {
                    // We need to collect after space info from the nodes which
                    // have layouts.
                    if (pElement->IsOwnLineElement(pFlowLayout))
                    {
                        pNodeForAfterSpace = pNode;
                    }
                    break;
                }
                else if (pElement->Tag() == ETAG_BR)
                    break;
            }
            else if (ptp->IsBeginElementScope())
            {
                if (    pFlowLayout->IsElementBlockInContext(pElement)
                    ||  !pNode->Element()->IsNoScope())
                    break;
                else if (   pElement->_fBreakOnEmpty
                         && pFlowLayout->IsElementBlockInContext(pElement)
                        )
                    break;
            }
        }
        else
        {
            Assert(ptp->IsText());
            if (ptp->Cch())
                break;
        }
    }

    Assert(ptpNext);
    
Cleanup:
    if (pptp)
        *pptp = ptpNext;
    if (pcch)
    {
        if (ptpNext == ptpOriginal)
            *pcch = 0;
        else
            *pcch = cpEndForLine - ptpNext->GetCp();
    }
    if (ppNodeForAfterSpace)
        *ppNodeForAfterSpace = pNodeForAfterSpace;
    
    return ptpNext->GetBranch();
}

long
ComputeLineShift(htmlAlign  atAlign,
                 BOOL       fRTLDisplay,
                 BOOL       fRTLLine,
                 BOOL       fMinMax,
                 long       xWidthMax,
                 long       xWidth,
                 UINT *     puJustified,
                 long *     pdxRemainder)
{
    long xShift = 0;
    long xRemainder = 0;

    switch(atAlign)
    {
    case htmlAlignNotSet:
        if(fRTLDisplay == fRTLLine)
            *puJustified = JUSTIFY_LEAD;
        else
            *puJustified = JUSTIFY_TRAIL;
        break;

    case htmlAlignLeft:
        if(!fRTLDisplay)
        {
            *puJustified = JUSTIFY_LEAD;
        }
        else
        {
            *puJustified = JUSTIFY_TRAIL;
        }
        break;

    case htmlAlignRight:
        if(!fRTLDisplay)
        {
            *puJustified = JUSTIFY_TRAIL;
        }
        else
        {
            *puJustified = JUSTIFY_LEAD;
        }
        break;

    case htmlAlignCenter:
        *puJustified = JUSTIFY_CENTER;
        break;

    case htmlBlockAlignJustify:
        if(!fRTLDisplay)
        {
            if(!fRTLLine)
                *puJustified = JUSTIFY_FULL;
            else
                *puJustified = JUSTIFY_TRAIL;
        }
        else
            *puJustified = JUSTIFY_FULL;
        break;

    default:
        AssertSz(FALSE, "Did we introduce new type of alignment");
        break;
    }
    if (    !fMinMax
        &&  *puJustified != JUSTIFY_FULL)
    {
        if (*puJustified != JUSTIFY_LEAD)
        {
            // for pre whitespace is already include in _xWidth
            xShift = xWidthMax - xWidth;

            xShift = max(xShift, 0L);           // Don't allow alignment to go < 0
                                                // Can happen with a target device
            if (*puJustified == JUSTIFY_CENTER)
            {
                Assert(atAlign == htmlAlignCenter);
                xShift /= 2;
            }
        }
        xRemainder = xWidthMax - xWidth - xShift;
    }

    Assert(pdxRemainder != NULL);
    *pdxRemainder = xRemainder;

    return xShift;
}

extern CDispNode * EnsureContentNode(CDispNode * pDispContainer);

HRESULT
CDisplay::InsertNewContentDispNode(CDispNode *  pDNBefore,
                                   CDispNode ** ppDispContent,
                                   long         iLine,
                                   long         yHeight)
{
    HRESULT       hr  = S_OK;
    CFlowLayout * pFlowLayout     = GetFlowLayout();
    CDispNode   * pDispContainer  = pFlowLayout->GetElementDispNode(); 
    CDispNode   * pDispNewContent = NULL;
    BOOL          fRightToLeft;

    Assert(pDispContainer);
    
    fRightToLeft = pDispContainer->IsRightToLeft();

    //
    // if a content node is not created yet, ensure that we have a content node.
    //
    if (!pDNBefore)
    {
        pDispContainer = pFlowLayout->EnsureDispNodeIsContainer();

        EnsureContentNode(pDispContainer);

        pDNBefore = pFlowLayout->GetFirstContentDispNode();

        Assert(pDNBefore);

        if (!pDNBefore)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        *ppDispContent = pDNBefore;
    }

    Assert(pDispContainer->IsContainer());

    //
    // Create a new content dispNode and size the previous dispNode
    //

    pDispNewContent = CDispRoot::CreateDispItemPlus(pFlowLayout,
                                                    TRUE,
                                                    FALSE,
                                                    FALSE,
                                                    DISPNODEBORDER_NONE,
                                                    fRightToLeft);

    if (!pDispNewContent)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pDispNewContent->SetPosition(CPoint(0, yHeight));
    pDispNewContent->SetSize(CSize(_xWidthView, 1), FALSE);

    if (fRightToLeft)
        pDispNewContent->FlipBounds();

    pDispNewContent->SetVisible(pDispContainer->IsVisible());
    pDispNewContent->SetExtraCookie((void *)(DWORD_PTR)(iLine));
    pDispNewContent->SetLayerType(DISPNODELAYER_FLOW);

    pDNBefore->InsertNextSiblingNode(pDispNewContent);

    *ppDispContent = pDispNewContent;
    
Cleanup:
    RRETURN(hr);
}

HRESULT
CDisplay::HandleNegativelyPositionedContent(CLine       * pliNew,
                                            CLSMeasurer * pme,
                                            CDispNode   * pDNBefore,
                                            long          iLinePrev,
                                            long          yHeight)
{
    HRESULT     hr = S_OK;
    CDispNode * pDNContent = NULL;

    Assert(pliNew);

    NoteMost(pliNew);

    if (iLinePrev > 0)
    {
        long yLineTop = pliNew->GetYTop();

        //
        // Create and insert a new content disp node, if we have negatively
        // positioned content.
        //
        
        // NOTE(SujalP): Changed from GetYTop to _yBeforeSpace. The reasons are
        // outlined in IE5 bug 62737.
        if (pliNew->_yBeforeSpace < 0 && !pliNew->_fDummyLine)
        {
            hr = InsertNewContentDispNode(pDNBefore, &pDNContent, iLinePrev, yHeight + yLineTop);

            if (hr)
                goto Cleanup;

            _fHasMultipleTextNodes = TRUE;

            if (pDNBefore == pme->_pDispNodePrev)
                pme->_pDispNodePrev = pDNContent;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------------
//
//  Member : ElementResize
//
//  Synopsis : CDisplay helper function to resize the element when the text content
//      has been remeasured and the container needs to be resized
//
//--------------------------------------------------------------------------------
void
CDisplay::ElementResize(CFlowLayout * pFlowLayout)
{
    if (!pFlowLayout)
        return;

    // If our contents affects our size, ask our parent to initiate a re-size
    if (    pFlowLayout->GetAutoSize()
        ||  pFlowLayout->_fContentsAffectSize)
    {
        pFlowLayout->ElementOwner()->ResizeElement();
    }

}
