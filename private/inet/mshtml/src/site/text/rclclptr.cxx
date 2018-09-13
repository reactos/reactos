//+------------------------------------------------------------------------
//
//  Class:      CRecalcLinePtr implementation
//
//  Synopsis:   Special line pointer. Encapsulate the use of a temporary
//              line array when building lines. This pointer automatically
//              switches between the main and the temporary new line array
//              depending on the line index.
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_LSM_HXX_
#define X_LSM_HXX_
#include "lsm.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_MARQUEE_HXX_
#define X_MARQUEE_HXX_
#include "marquee.hxx"
#endif

#ifndef X_RCLCLPTR_HXX_
#define X_RCLCLPTR_HXX_
#include "rclclptr.hxx"
#endif

MtDefine(CRecalcLinePtr, Layout, "CRecalcLinePtr")
MtDefine(CRecalcLinePtr_aryLeftAlignedImgs_pv, CRecalcLinePtr, "CRecalcLinePtr::_aryLeftAlignedImgs::_pv")
MtDefine(CRecalcLinePtr_aryRightAlignedImgs_pv, CRecalcLinePtr, "CRecalcLinePtr::_arRightAlignedImgs::_pv")

#pragma warning(disable:4706) /* assignment within conditional expression */

//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::CRecalcLinePtr
//
//  Synopsis:   constructor, initializes (caches) margins for the current
//              display
//
//-------------------------------------------------------------------------

CRecalcLinePtr::CRecalcLinePtr(CDisplay *pdp, CCalcInfo *pci)
    : _aryLeftAlignedImgs(Mt(CRecalcLinePtr_aryLeftAlignedImgs_pv)),
      _aryRightAlignedImgs(Mt(CRecalcLinePtr_aryRightAlignedImgs_pv))
{
    CFlowLayout *   pFlowLayout = pdp->GetFlowLayout();
    CElement *      pElementFL  = pFlowLayout->ElementContent();
    long            lPadding[PADDING_MAX];

    WHEN_DBG( _cAll = -1; )

    _pdp = pdp;
    _pci = pci;
    _iPF = -1;
    _fInnerPF = FALSE;
    _xLeft       =
    _xRight      =
    _yBordTop    = 
    _xBordLeft   =
    _xBordRight  =
    _yBordBottom = 0;
    _xPadLeft    =
    _yPadTop     =
    _xPadRight   =
    _yPadBottom  = 0;

    // I am not zeroing out the following because it is not necessary. We zero it out
    // everytime we call CalcBeforeSpace
    // _yBordLeftPerLine = _xBordRightPerLine = _xPadLeftPerLine = _xPadRightPerLine = 0;
    
    ResetPosAndNegSpace();

    _cLeftAlignedLayouts =
    _cRightAlignedLayouts = 0;
    _fIsEditable = pFlowLayout->IsEditable();
    
    if(pElementFL->Tag() == ETAG_MARQUEE &&
        !_fIsEditable &&
        !_pdp->_fPrinting)
    {
        _xMarqueeWidth = DYNCAST(CMarquee, pElementFL)->_lXMargin;
    }
    else
    {
        _xMarqueeWidth  = 0;
    }

    _pdp->GetPadding(pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH);
    _xLayoutLeftIndent  = lPadding[PADDING_LEFT];
    _xLayoutRightIndent = lPadding[PADDING_RIGHT];
    _fNoMarginAtBottom = FALSE;
}


//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::Init
//
//  Synopsis:   Initialize the old and new line array and reset the
//              RecalcLineptr.
//
//-------------------------------------------------------------------------

void CRecalcLinePtr::Init(CLineArray * prgliOld, int iNewFirst, CLineArray * prgliNew)
{
    _prgliOld = prgliOld;
    _prgliNew = prgliNew;
    _xMaxRightAlign = 0;
    Reset(iNewFirst);
}

//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::Reset
//
//  Synopsis:   Resets the RecalcLinePtr to use the given offset. Look
//              for all references to line >= iNewFirst to be looked up in the
//              new line array else in the old line array.
//
//-------------------------------------------------------------------------

void CRecalcLinePtr::Reset(int iNewFirst)
{
    _iNewFirst = iNewFirst;
    _iLine = 0;
    _iNewPast = _prgliNew ? _iNewFirst + _prgliNew->Count() : 0;
    _cAll = _prgliNew ? _iNewPast : _prgliOld->Count();
    _pPrevBlockNode = _pPrevRunOwnerBranch = NULL;
    Assert(_iNewPast <= _cAll);
}


//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::operator
//
//  Synopsis:   returns the line from the old or the new line array based
//              on _iNewFirst.
//
//-------------------------------------------------------------------------

CLine * CRecalcLinePtr::operator [] (int iLine)
{
    Assert(iLine < _cAll);
    if (iLine >= _iNewFirst && iLine < _iNewPast)
    {
        return _prgliNew->Elem(iLine - _iNewFirst);
    }
    else
    {
        return _prgliOld->Elem(iLine);
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::AddLine
//
//  Synopsis:   Add's a new line at the end of the new line array.
//
//-------------------------------------------------------------------------

CLine * CRecalcLinePtr::AddLine()
{
    CLine * pLine = _prgliNew ? _prgliNew->Add(1, NULL): _prgliOld->Add(1, NULL);
    if(pLine)
        Reset(_iNewFirst);  // Update _cAll, _iNewPast, etc. to reflect
                            // the correct state after adding line
    return pLine;
}


//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::InsertLine
//
//  Synopsis:   Inserts a line to the old or new line array before the given
//              line.
//
//-------------------------------------------------------------------------

CLine * CRecalcLinePtr::InsertLine(int iLine)
{
    CLine * pLine = _prgliNew ? _prgliNew->Insert(iLine - _iNewFirst, 1):
                                _prgliOld->Insert(iLine, 1);
    if(pLine)
        Reset(_iNewFirst);  // Update _cAll, _iNewPast, etc. to reflect
                            // the correct state after the newly inserted line
    return pLine;
}


//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::First
//
//  Synopsis:   Sets the iLine to be the current line and returns it
//
//  Returns:    returns iLine'th line if there is one.
//
//-------------------------------------------------------------------------

CLine * CRecalcLinePtr::First(int iLine)
{
    _iLine = iLine;
    if (_iLine < _cAll)
        return (*this)[_iLine];
    else
        return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::Next
//
//  Synopsis:   Moves the current line to the next line, if there is one
//
//  Returns:    returns the next line if there is one.
//
//-------------------------------------------------------------------------

CLine * CRecalcLinePtr::Next()
{
    if (_iLine + 1 < _cAll)
        return (*this)[++_iLine];
    else
        return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::Prev
//
//  Synopsis:   Moves the current line to the previous line, if there is one
//
//  Returns:    returns the previous line if there is one.
//
//-------------------------------------------------------------------------

CLine * CRecalcLinePtr::Prev()
{
    if (_iLine > 0)
        return (*this)[--_iLine];
    else
        return NULL;
}


//+----------------------------------------------------------------------------
//
// Member:      CRecalcLinePtr::InitPrevAfter ()
//
// Synopsis:    Initializes the after spacing of the previous line's paragraph
//
//-----------------------------------------------------------------------------
void CRecalcLinePtr::InitPrevAfter(BOOL *pfLastWasBreak, CLinePtr& rpOld)
{
    int oldLine;

    *pfLastWasBreak = FALSE;

    // Now initialize the linebreak stuff.
    oldLine = rpOld;
    if (rpOld.PrevLine(TRUE, FALSE))
    {
        // If we encounter a break in the previous line, then we
        // need to remember that for the accumulation to work.
        if (rpOld->_fHasBreak)
        {
            *pfLastWasBreak = TRUE;
        }
    }

    rpOld = oldLine;
}


//+------------------------------------------------------------------------
//
//  Member:     ApplyLineIndents
//
//  Synopsis:   Apply left and right indents for the current line.
//
//-------------------------------------------------------------------------
void
CRecalcLinePtr::ApplyLineIndents(
    CTreeNode * pNode,     
    CLine *pLineMeasured,
    UINT uiFlags)
{
    LONG        xLeft;      // Use logical units
    LONG        xRight;     // Use logical units
    long        iPF;
    const CParaFormat * pPF;
    CFlowLayout   * pFlowLayout = _pdp->GetFlowLayout();
    CElement      * pElementFL  = pFlowLayout->ElementContent();
    BOOL            fInner      = SameScope(pNode, pElementFL);
    BOOL            fRTLDisplay = _pdp->IsRTL();

    iPF = pNode->GetParaFormatIndex();
    pPF = GetParaFormatEx(iPF);
    BOOL fRTLLine = pPF->HasRTL(TRUE);

    if (   _iPF != iPF
        || _fInnerPF != fInner
       )
    {
        _iPF    = iPF;
        _fInnerPF = fInner;
        _xLeft  = pPF->GetLeftIndent(_pci, fInner);
        _xRight = pPF->GetRightIndent(_pci, fInner);

        // If we have horizontal indents in percentages, flag the display
        // so it can do a full recalc pass when necessary (e.g. parent width changes)
        // Also see CalcBeforeSpace() where we do this for horizontal padding.
        if ( (pPF->_cuvLeftIndentPercent.IsNull() ? FALSE :
              pPF->_cuvLeftIndentPercent.GetUnitValue() )   ||
             (pPF->_cuvRightIndentPercent.IsNull() ? FALSE :
              pPF->_cuvRightIndentPercent.GetUnitValue() ) )
        {
            _pdp->_fContainsHorzPercentAttr = TRUE;
        }
    }

    xLeft = _xLeft;
    xRight = _xRight;

    // (changes in the next section should be reflected in AlignObjects() too)
    
    // Adjust xLeft to account for marquees, padding and borders.
    xLeft += _xMarqueeWidth + _xLayoutLeftIndent;
    xRight += _xMarqueeWidth + _xLayoutRightIndent;
    
    xLeft  += _xPadLeft + _xBordLeft;
    xRight += _xPadRight + _xBordRight;
    
    if(!fRTLDisplay)
        xLeft += _xLeadAdjust;
    else
        xRight += _xLeadAdjust;

    // xLeft is now the sum of indents, border, padding.  CSS requires that when
    // possible, this indent is shared with the space occupied by floated/ALIGN'ed
    // elements (our code calls that space "margin").  Thus we want to apply a +ve
    // xLeft only when it's greater than the margin, and the amount applied excludes
    // what's occupied by the margin already.  (We never want to apply a -ve xLeft)
    // Same reasoning applies to xRight.
    // Note that xLeft/xRight has NOT accumulated CSS text-indent values yet;
    // this is because we _don't_ want that value to be shared the way the above
    // values have been shared.  We'll factor in text-indent after this.
    if (_marginInfo._xLeftMargin)
        pLineMeasured->_xLeft = max( 0L, xLeft - _marginInfo._xLeftMargin );
    else
        pLineMeasured->_xLeft = xLeft;

    if (_marginInfo._xRightMargin)
        pLineMeasured->_xRight = max( 0L, xRight - _marginInfo._xRightMargin );
    else
        pLineMeasured->_xRight = xRight;

    // text indent is inherited, so if the formatting node correspond's to a layout,
    // indent the line only if the layout is not a block element. For layout element's
    // the have blockness, text inherited and first line of paragraph's inside are
    // indented.
    if (    uiFlags & MEASURE_FIRSTINPARA
        && (   pNode->Element() == pElementFL
            || !pNode->HasLayout()
            || !pNode->Element()->IsBlockElement())
       )
    {
        if(!fRTLLine)
            pLineMeasured->_xLeft += pPF->GetTextIndent(_pci);
        else
            pLineMeasured->_xRight += pPF->GetTextIndent(_pci);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CalcInterParaSpace
//
//  Synopsis:   Calculate space before the current line.
//
//  Arguments:  [pMe]              --
//              [iLineFirst]       --  line to start calculating from
//              [yHeight]          --  y coordinate of the top of line
//
//
//-------------------------------------------------------------------------
CTreeNode *
CRecalcLinePtr::CalcInterParaSpace(CLSMeasurer * pMe, LONG iPrevLine, BOOL fFirstLineInLayout)
{
    LONG iLine;
    CLine *pPrevLine = NULL;
    CTreeNode * pNodeFormatting;

    INSYNC(pMe);
    
    // Get the previous line that's on a different physical line
    // and is not a frame line. Note that the initial value of
    // iPrevLine IS the line before the one we're about to measure.
    for (iLine = iPrevLine; iLine >= 0; --iLine)
    {
        pPrevLine = (*this)[iLine];

        if (pPrevLine->_fForceNewLine && pPrevLine->_cch > 0)
            break;
    }
    
    //pMe->MeasureSetPF(pPF);
    //pMe->_pLS->_fInnerPFFirst = SameScope(pMe->CurrBranch(), _pdp->GetFlowLayoutElement());

    pNodeFormatting = CalcParagraphSpacing(pMe, fFirstLineInLayout);

    // If a line consists of only a BR character and a block tag,
    // fold the previous line height into the before space, too.
    if (pPrevLine && pPrevLine->_fEatMargin)
    {
        //
        // NOTE(SujalP): When we are eating margin we have to be careful to
        // take into account both +ve and -ve values for both LineHeight
        // yBS. The easiest way to envision this is to think both of them
        // as margins which need to be merged -- which is similar to the
        // case where we are computing before and after space. Hence the
        // code here is similar to the code one would seen in CalcBeforeSpace
        // where we set up up _yPosSpace and _yNegSpace.
        //
        // If you have the urge to change this code please make sure you
        // do not change the behaviour of 38561 and 61132.
        //
        LONG yPosSpace;
        LONG yNegSpace;
        LONG yBeforeSpace = pMe->_li._yBeforeSpace;
        
        yPosSpace = max(0L, pPrevLine->_yHeight);
        yPosSpace = max(yPosSpace, yBeforeSpace);

        yNegSpace = min(0L, pPrevLine->_yHeight);
        yNegSpace = min(yNegSpace, yBeforeSpace);

        pMe->_li._yBeforeSpace = SHORT((yPosSpace + yNegSpace) - pPrevLine->_yHeight);
    }

    INSYNC(pMe);
    
    return pNodeFormatting;
}

/*
 *  CRecalcLinePtr::NetscapeBottomMargin(pMe, pDI)
 *
 *  This function is called for the last line in the display.
 *  It exists because Netscape displays by streaming tags and it
 *  increases the height of a table cell when an end tag passes
 *  by without determining if there is any more text. This makes
 *  it impossible to have a bunch of cells with headers in them
 *  where the cells are tight around the text, but c'est la vie.
 *
 *  We store the extra space in an intermediary and then add it
 *  into the bottom margin later.
 *
 */
LONG
CRecalcLinePtr::NetscapeBottomMargin(CLSMeasurer * pMe)
{
    if (   _fNoMarginAtBottom
        || _pdp->GetFlowLayout()->ElementContent()->Tag() == ETAG_BODY
       )
    {
        ResetPosAndNegSpace();
        _fNoMarginAtBottom = FALSE;
    }
    
    // For empty paragraphs at the end of a layout, we create a dummy line
    // so we can just use the before space of that dummy line. If there is 
    // no dummy line, then we need to use the after space of the previous 
    // line.
    return pMe->_li._fDummyLine
            ? pMe->_li._yBeforeSpace
            : _lTopPadding + _lNegSpaceNoP + _lPosSpaceNoP;
}


//+------------------------------------------------------------------------
//
//  Member:     RecalcMargins
//
//  Synopsis:   Calculate new margins for the current line.
//
//  Arguments:  [iLineStart]       --  new lines start at
//              [iLineFirst]       --  line to start calculating from
//              [yHeight]          --  y coordinate of the top of line
//              [yBeforeSpace]     --  before space of the current line
//
//-------------------------------------------------------------------------
void CRecalcLinePtr::RecalcMargins(
    int iLineStart,
    int iLineFirst,
    LONG yHeight,
    LONG yBeforeSpace)
{
    LONG            y;
    CLine *         pLine;
    int             iAt;
    CFlowLayout   * pFlowLayout = _pdp->GetFlowLayout();
    LONG            xWidth      = pFlowLayout->GetMaxLineWidth();

    // Initialize margins to defaults.
    _marginInfo.Init();
    _fMarginFromStyle = FALSE;

    _pNodeLeftTop    = NULL;
    _pNodeRightTop   = NULL;

    // Update the state of the line array.
    Reset(iLineStart);

    // go back to find the first line which is clear (i.e., a line
    // with default margins)
    y = yHeight;
    iAt = -1;

    First(iLineFirst);

    for (pLine = Prev(); pLine; pLine = Prev())
    {
        if (pLine->IsFrame())
        {
            // Cache the line which is aligned
            iAt = At();
        }
        else
        {
            if (pLine->HasMargins())
            {
                // current line has margins
                if(pLine->_fForceNewLine)
                    y -= pLine->_yHeight;
            }
            else
                break;
        }
    }

    // iAt now holds the last aligned frame line we saw while walking back,
    // and pLine is either NULL or points to the first clear line (i.e.,
    // no margins were specified).  The Rclclptr state (_iLine) also indexes
    // that clear line.

    // y is the y coordinate of first clear line.

    // if there were no frames there is nothing to do...
    if (iAt == -1)
        return;

    // There are aligned sites, so calculate margins.
    int             iLeftAlignedImages = 0;
    int             iRightAlignedImages = 0;
    CAlignedLine *  pALine;
    CLine *         pLineLeftTop = NULL;
    CLine *         pLineRightTop = NULL;

    // stacks to keep track of the number of left & right aligned sites
    // adding margins to the current line.
    _aryLeftAlignedImgs.SetSize(0);
    _aryRightAlignedImgs.SetSize(0);

    // have to adjust the height of the aligned frames.
    // iAt is the last frame line we saw, so start there and
    // walk forward until we get to iLineFirst.

    // _yBottomLeftMargin and _yBottomRightMargin are the max y values
    // for which the current left/right margin values are valid; i.e.,
    // once y increases past _yBottomLeftMargin, there's a new left margin
    // that must be used.
    
    for (pLine = First(iAt);
        pLine && At() <= iLineFirst;
        pLine = Next())
    {

        // update y to skip by 
        if(At() == iLineFirst)
            y += yBeforeSpace;
        else
        {
            // Include the text or center aligned height
            if (pLine->_fForceNewLine)
            {
                y += pLine->_yHeight;
            }
        }

        // If there are any left aligned images
        while(iLeftAlignedImages)
        {
            // check to see if we passed the left aligned image, if so pop it of the
            // stack and set its height
            pALine = & _aryLeftAlignedImgs [ iLeftAlignedImages - 1 ];

            if(y >= pALine->_pLine->_yHeight + pALine->_yLine)
            {
                _aryLeftAlignedImgs.Delete( -- iLeftAlignedImages );
            }
            else
                break;
        }
        // If there are any right aligned images
        while(iRightAlignedImages)
        {
            pALine = & _aryRightAlignedImgs [ iRightAlignedImages - 1 ];

            // check to see if we passed the right aligned image, if so pop it of the
            // stack and set its height
            if(y >= pALine->_pLine->_yHeight + pALine->_yLine)
            {
                _aryRightAlignedImgs.Delete( -- iRightAlignedImages );
            }
            else
                break;
        }

        if (pLine->IsFrame() && At() != iLineFirst)
        {
            HRESULT hr = S_OK;
            const CFancyFormat * pFF = pLine->_pNodeLayout->GetFancyFormat();
            CAlignedLine al;

            al._pLine = pLine;
            al._yLine = y;

            // If the current line is left aligned push it on to the left aligned images
            // stack and adjust the _yBottomLeftMargin.
            if (pLine->IsLeftAligned())
            {
                //
                // If the current aligned element needs to clear left, insert
                // the current aligned layout below all other left aligned
                // layout's since it is the last element that establishes
                // the margin.
                //
                if (pFF->_fClearLeft)
                {
                    hr = _aryLeftAlignedImgs.InsertIndirect(0, &al);

                    if (hr)
                        return;
                }
                else
                {
                    CAlignedLine *  pAlignedLine;

                    hr = _aryLeftAlignedImgs.AppendIndirect(NULL, &pAlignedLine);

                    if (hr)
                        return;

                    *pAlignedLine = al;
                }

                iLeftAlignedImages++;

                if (y + pLine->_yHeight > _marginInfo._yBottomLeftMargin)
                {
                    _marginInfo._yBottomLeftMargin = y + pLine->_yHeight;
                }
            }
            // If the current line is right aligned push it on to the right aligned images
            // stack and adjust the _yBottomRightMargin.
            else if (pLine->IsRightAligned())
            {
                //
                // If the current aligned element needs to clear right, insert
                // the current aligned layout below all other right aligned
                // layout's since it is the last element that establishes
                // the margin.
                //
                if (pFF->_fClearRight)
                {
                    hr = _aryRightAlignedImgs.InsertIndirect(0, &al);

                    if (hr)
                        return;
                }
                else
                {
                    CAlignedLine *  pAlignedLine;

                    hr = _aryRightAlignedImgs.AppendIndirect(NULL, &pAlignedLine);

                    if (hr)
                        return;

                    *pAlignedLine = al;
                }

                iRightAlignedImages++;

                if (y + pLine->_yHeight > _marginInfo._yBottomRightMargin)
                {
                    _marginInfo._yBottomRightMargin = y + pLine->_yHeight;
                }
            }
        }
    }

    // Only use remaining floated objects.
    _fMarginFromStyle = FALSE;

    // If we have any left aligned sites left on the stack, calculate the
    // left margin.
    if(iLeftAlignedImages)
    {
        // get the topmost left aligned line and compute the current lines
        // left margin
        pALine = & _aryLeftAlignedImgs [ iLeftAlignedImages - 1 ];
        pLineLeftTop = pALine->_pLine;
        _pNodeLeftTop = pLineLeftTop->_pNodeLayout;
        if(!_pdp->IsRTL())
            _marginInfo._xLeftMargin = pLineLeftTop->_xLeftMargin + pLineLeftTop->_xLineWidth;
        else
            _marginInfo._xLeftMargin = max(0L, xWidth - pLineLeftTop->_xRightMargin);
        _marginInfo._yLeftMargin = pLineLeftTop->_yHeight + pALine->_yLine;

        // Note if a "frame" margin may be needed
        _marginInfo._fAddLeftFrameMargin = pLineLeftTop->_fAddsFrameMargin;

        // Note whether margin space is due to CSS float (as opposed to ALIGN attr).
        // We need to know this because if it's CSS float, we autoclear after the line.
        _fMarginFromStyle |= pLineLeftTop->_pNodeLayout->GetFancyFormat()->_fCtrlAlignFromCSS;
    }
    else
    {
        // no left aligned sites for the current line, so set the margins to
        // defaults.
        _marginInfo._yBottomLeftMargin = MINLONG;
        _marginInfo._yLeftMargin = MAXLONG;
        _marginInfo._xLeftMargin = 0;
    }
    // If we have any right aligned sites left on the stack, calculate the
    // right margin.
    if(iRightAlignedImages)
    {
        // get the topmost right aligned line and compute the current lines
        // right margin
        pALine = & _aryRightAlignedImgs [ iRightAlignedImages - 1 ];
        pLineRightTop = pALine->_pLine;
        _pNodeRightTop = pLineRightTop->_pNodeLayout;
        if(!_pdp->IsRTL())
            _marginInfo._xRightMargin = max(0L, xWidth - pLineRightTop->_xLeftMargin);
        else
            _marginInfo._xRightMargin = pLineRightTop->_xRightMargin + pLineRightTop->_xLineWidth;
        _marginInfo._yRightMargin = pLineRightTop->_yHeight + pALine->_yLine;

        // Note if a "frame" margin may be needed
        _marginInfo._fAddRightFrameMargin = pLineRightTop->_fAddsFrameMargin;

        // Note whether margin space is due to CSS float (as opposed to ALIGN attr).
        // We need to know this because if it's CSS float, we autoclear after the line.
        _fMarginFromStyle |= pLineRightTop->_pNodeLayout->GetFancyFormat()->_fCtrlAlignFromCSS;
    }
    else
    {
        // no right aligned sites for the current line, so set the margins to
        // defaults.
        _marginInfo._yBottomRightMargin = MINLONG;
        _marginInfo._yRightMargin = MAXLONG;
        _marginInfo._xRightMargin = 0;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     AlignObjects
//
//  Synopsis:   Process all aligned objects on a line and create new
//              "frame" lines for them.
//
//  Arguments:  [pMe]              --  measurer used to recalc lines
//              [uiFlags]          --  flags
//              [iLineStart]       --  new lines start at
//              [iLineFirst]       --  line to start calculating from
//              [pyHeight]         --  y coordinate of the top of line
//
//-------------------------------------------------------------------------
int CRecalcLinePtr::AlignObjects(
    CLSMeasurer *pme,
    CLine       *pLineMeasured,
    LONG         cch,
    BOOL         fMeasuringSitesAtBOL,
    BOOL         fBreakAtWord,
    BOOL         fMinMaxPass,
    int          iLineStart,
    int          iLineFirst,
    LONG        *pyHeight,
    int          xWidthMax,
    LONG        *pyAlignDescent,
    LONG        *pxMaxLineWidth)
{
    CFlowLayout *       pFlowLayout = _pdp->GetFlowLayout();
    CLayout *           pLayout;
    LONG                xWidth      = pFlowLayout->GetMaxLineWidth();
    BOOL                fBrowseMode = !_fIsEditable;
    htmlControlAlign    atSite;
    CLine *             pLine;
    CLine *             pLineNew;
    LONG                yHeight;
    LONG                yHeightCurLine = 0;
    int                 xMin = 0;
    int                 iClearLines=0;
    CTreePos           *ptp;
    CTreeNode          *pNodeLayout;
    LONG                cchInTreePos;
    BOOL                fRTLDisplay = _pdp->IsRTL();
    
    Reset(iLineStart);
    pLine = (*this)[iLineFirst];

    // Make sure the current line has aligned sites
    Assert(pLine->_fHasAligned);

    yHeight = *pyHeight;

    // If we are measuring sites at the beginning of the line,
    // measurer is positioned at the current site. So we dont
    // need to back up the measurer. Also, margins the current
    // line apply to the line being inserted before the current
    // line.
    if(!fMeasuringSitesAtBOL)
    {
        // fliForceNewLine is set for lines that have text in them.
        Assert (pLine->_fForceNewLine);

        // adjust the height and recalc the margins for the aligned
        // lines being inserted after the current line
        yHeightCurLine = pLine->_yHeight;
        yHeight += yHeightCurLine;

        iLineFirst++;
        if (!IsValidMargins(yHeight))
        {
            RecalcMargins(iLineStart, iLineFirst, yHeight, 0);
        }
    }

    ptp = pme->GetPtp();
    pNodeLayout = ptp->GetBranch();
    if (ptp->IsText())
        cchInTreePos = ptp->Cch() - (pme->GetCp() - ptp->GetCp());
    else
        cchInTreePos = ptp->GetCch();
    while (cch > 0)
    {
        if (ptp->IsBeginElementScope())
        {
            pNodeLayout = ptp->Branch();
            if (pNodeLayout->NeedsLayout())
            {
                const CCharFormat  * pCF = pNodeLayout->GetCharFormat();
                const CParaFormat  * pPF = pNodeLayout->GetParaFormat();
                const CFancyFormat * pFF = pNodeLayout->GetFancyFormat();
                BOOL        fClearLeft      = pFF->_fClearLeft;
                BOOL        fClearRight     = pFF->_fClearRight;
                CElement *  pElementLayout  = pNodeLayout->Element();
                
                cchInTreePos = pme->GetNestedElementCch(pElementLayout, &ptp);
                
                atSite = pNodeLayout->GetSiteAlign();

                if (   (!fBrowseMode || !pCF->IsDisplayNone())
                    && pFF->_fAlignedLayout
                   )
                {
                    LONG xWidthSite, yHeightSite;
                    
                    pLayout = pNodeLayout->GetUpdatedLayout();
                    
                    // measuring sites at the Beginning of the line,
                    // insert the aligned line before the current line
                    if(fMeasuringSitesAtBOL)
                    {
                        pLineNew = InsertLine(iLineFirst);

                        if (!pLineNew)
                            goto Cleanup;

                        pLineNew->_fFrameBeforeText = TRUE;
                        pLineNew->_yBeforeSpace = pLineMeasured->_yBeforeSpace;
                    }
                    else
                    {
                        // insert the aligned line after the current line
                        pLineNew = AddLine();

                        if (!pLineNew)
                            goto Cleanup;

                        pLineNew->_fHasEOP = (*this)[iLineFirst - 1]->_fHasEOP;
                    }

                    // Update the line width and new margins caused by the aligned line
                    for(;;)
                    {
                        // Measure the site
                        pFlowLayout->GetSiteWidth(pLayout, _pci, fBreakAtWord,
                                        xWidthMax - _xLayoutLeftIndent - _xLayoutRightIndent,
                                        &xWidthSite, &yHeightSite, &xMin);

                        //
                        // if clear is set on the layout, we don need to auto clear.
                        //
                        if (fClearLeft || fClearRight)
                            break;

                        // If we've overflowed, and the current margin allows for auto
                        // clearing, we have to introduce a clear line.
                        if (_fMarginFromStyle &&
                            _marginInfo._xLeftMargin + _marginInfo._xRightMargin > 0 &&
                            xWidthSite > xWidthMax)
                        {
                            int iliClear;

                            _marginInfo._fAutoClear = TRUE;

                            // Find the index of the clear line that is being added.
                            if(fMeasuringSitesAtBOL)
                            {
                                iliClear = iLineFirst + iClearLines;
                            }
                            else
                            {
                                iliClear = Count() - 1;
                            }

                            // add a clear line
                            ClearObjects(pLineNew, iLineStart,
                                         iliClear,
                                         &yHeight);

                            iClearLines++;

                            //
                            // Clear line takes any beforespace, so clear the
                            // beforespace for the current line
                            //
                            pme->_li._yBeforeSpace = 0;
                            ResetPosAndNegSpace();

                            // insert the new line for the alined element after the
                            // clear line.
                            if(fMeasuringSitesAtBOL)
                            {
                                pLineNew = InsertLine(iLineFirst + iClearLines);

                                if (!pLineNew)
                                    goto Cleanup;

                                pLineNew->_fFrameBeforeText = TRUE;
                            }
                            else
                            {
                                pLineNew = AddLine();

                                if (!pLineNew)
                                    goto Cleanup;

                                pLineNew->_fFrameBeforeText = FALSE;
                            }

                            RecalcMargins(iLineStart,
                                          fMeasuringSitesAtBOL
                                            ? iLineFirst + iClearLines
                                            : Count() - 1,
                                          yHeight, 0);

                            xWidthMax = GetAvailableWidth();
                        }

                        // We fit just fine, but keep track of total available width.
                        else
                        {
                            xWidthMax -= xWidthSite;
                            break;
                        }
                    } // end of for loop

                    // Start out assuming that there are no style floated objects.
                    _fMarginFromStyle = FALSE;

                    // Note if the site adds "frame" margin space
                    // (e.g., tables do, images do not)
                    pLineNew->_fAddsFrameMargin = !(pNodeLayout->Element()->Tag() == ETAG_IMG);

                    // Top Margin is included in the line height for aligned lines,
                    // at the top of the document. Before paragraph spacing is
                    // also included in the line height.

                    long xLeftMargin, yTopMargin, xRightMargin;
                    // get the site's margins
                    pLayout->GetMarginInfo(_pci, &xLeftMargin, &yTopMargin, &xRightMargin, NULL);

                    long xPos;

                    if(!fRTLDisplay)
                        xPos = xLeftMargin;
                    else
                        xPos = xRightMargin;

                    // Set proposed relative to the line
                    pLayout->SetXProposed(xPos);
                    pLayout->SetYProposed(yTopMargin + (fMeasuringSitesAtBOL ? (_yPadTop + _yBordTop) : 0));

                    if(pCF->HasCharGrid(FALSE))
                    {
                        long xGridWidthSite = pme->_pLS->GetClosestGridMultiple(pme->_pLS->GetCharGridSize(), xWidthSite);
                        pLayout->SetXProposed(xPos + ((xGridWidthSite - xWidthSite)/2));
                        xWidthSite = xGridWidthSite;
                    }

                    if(pCF->HasLineGrid(FALSE))
                    {
                        long yGridHeightSite = pme->_pLS->GetClosestGridMultiple(pme->_pLS->GetLineGridSize(), yHeightSite);
                        pLayout->SetYProposed(yTopMargin + ((yGridHeightSite - yHeightSite)/2));
                        yHeightSite = yGridHeightSite;
                    }

                    // _cch, _xOverhang and _xLeft are initialized to zero
                    Assert(pLineNew->_cch == 0 &&
                           pLineNew->_xLineOverhang == 0 &&
                           pLineNew->_xLeft == 0 &&
                           pLineNew->_xRight == 0);

                    // for the current line measure the left and the right indent
                    {
                        long  xLeftIndent   = pPF->GetLeftIndent(_pci, FALSE);
                        long  xRightIndent  = pPF->GetRightIndent(_pci, FALSE);

                        if (pCF->_fHasBgColor || pCF->_fHasBgImage)
                        {
                            pLineNew->_fHasBackground = TRUE;
                        }

                        // (changes to the next block should be reflected in
                        //  ApplyLineIndents() too)
                        if(atSite == htmlAlignLeft)
                        {
                            _marginInfo._fAddLeftFrameMargin = pLineNew->_fAddsFrameMargin;
                            
                            xLeftIndent += _xMarqueeWidth + _xLayoutLeftIndent;
                            xLeftIndent += _xPadLeft + _xBordLeft;

                            // xLeftIndent is now the sum of indents.  CSS requires that when possible, this
                            // indent is shared with the space occupied by floated/ALIGN'ed elements
                            // (our code calls that space "margin").  Thus we want to apply a +ve xLeftIndent
                            // only when it's greater than the margin, and the amount applied excludes
                            // what's occupied by the margin already.  (We never want to apply a -ve xLeftIndent)

                            if (!fClearLeft)
                                pLineNew->_xLeft = max( 0L, xLeftIndent - _marginInfo._xLeftMargin );
                            else
                                pLineNew->_xLeft = xLeftIndent;

                            // If the current layout has clear left, then we need to adjust its
                            // before space so we're beneath all other left-aligned layouts.
                            // For the current layout to be clear left, its yBeforeSpace + yHeight must
                            // equal or exceed the current yBottomLeftMargin.
                            if (    fClearLeft
                                && _marginInfo._xLeftMargin
                                &&  pLineNew->_yBeforeSpace < _marginInfo._yBottomLeftMargin - yHeight)
                            {
                                pLineNew->_yBeforeSpace = _marginInfo._yBottomLeftMargin - yHeight;
                            }
                        }
                        else if(atSite == htmlAlignRight)
                        {
                            // If it's right aligned, remember the widest one in max mode.
                            _xMaxRightAlign = max(_xMaxRightAlign, long(xMin));
                            
                            _marginInfo._fAddRightFrameMargin = pLineNew->_fAddsFrameMargin;
                            
                            xRightIndent += _xMarqueeWidth + _xLayoutRightIndent;
                            xRightIndent += _xPadRight + _xBordRight;

                            // xRightIndent is now the sum of indents.  CSS requires that when possible, this
                            // indent is shared with the space occupied by floated/ALIGN'ed elements
                            // (our code calls that space "margin").  Thus we want to apply a +ve xRightIndent
                            // only when it's greater than the margin, and the amount applied excludes
                            // what's occupied by the margin already.  (We never want to apply a -ve xRightIndent)

                            if (!fClearRight)
                                pLineNew->_xRight = max( 0L, xRightIndent - _marginInfo._xRightMargin );                            
                            else
                                pLineNew->_xRight = xRightIndent;

                            // If the current layout has clear right, then we need to adjust its
                            // before space so we're beneath all other right-aligned layouts.
                            // For the current layout to be clear right, its yBeforeSpace + yHeight must
                            // equal or exceed the current yBottomRightMargin.
                            
                            if (    fClearRight
                                &&  _marginInfo._xRightMargin
                                &&  pLineNew->_yBeforeSpace < _marginInfo._yBottomRightMargin - yHeight)
                            {
                                pLineNew->_yBeforeSpace = _marginInfo._yBottomRightMargin - yHeight;
                            }
                        }
                    }

                    if(fRTLDisplay)
                        pLineNew->_xLeft   += _xLeadAdjust;
                    else
                        pLineNew->_xRight  += _xLeadAdjust;

                    pLineNew->_xWidth       = xWidthSite;
                    pLineNew->_xLineWidth   = pLineNew->_xWidth;

                    if (fMeasuringSitesAtBOL)
                        pLineNew->_yExtent  = _yPadTop + _yBordTop;

                    pLineNew->_yExtent      += yHeightSite;
                    pLineNew->_yHeight      = pLineNew->_yExtent + pLineNew->_yBeforeSpace;

                    pLineNew->_pNodeLayout     = pNodeLayout;

                    _fMarginFromStyle = pFF->_fCtrlAlignFromCSS;

                    // Update the left and right margins
                    if (atSite == htmlAlignLeft)
                    {
                        pLineNew->SetLeftAligned();

                        _cLeftAlignedLayouts++;

                        pLineNew->_xLineWidth  += pLineNew->_xLeft;

                        if (!fRTLDisplay)
                        {
                            pLineNew->_xLeftMargin = fClearLeft ? 0 : _marginInfo._xLeftMargin;
                        }
                        else
                        {
                            if (fClearLeft)
                            {
                                pLineNew->_xLeftMargin = 0;
                            }

                            if (fMinMaxPass)
                            {
                                pLineNew->_xRightMargin = _marginInfo._xRightMargin;
                            }
                            else
                            {
                                pLineNew->_xRightMargin = max(_marginInfo._xRightMargin,
                                                           xWidth + 2 * _xMarqueeWidth -                                                       
                                                           (fClearLeft ? 0 : _marginInfo._xLeftMargin) -
                                                           pLineNew->_xLineWidth);
                                pLineNew->_xRightMargin = max(_xLayoutRightIndent,
                                                            pLineNew->_xRightMargin);
                            }
                        }

                        //
                        // if the current layout has clear then it may not
                        // establish the margin, because it needs to clear
                        // any left aligned layouts if any, in which case
                        // the left margin remains the same.
                        //
                        if (!fClearLeft || _marginInfo._xLeftMargin == 0 )
                        {
                            _marginInfo._xLeftMargin += pLineNew->_xLineWidth;
                            _marginInfo._yLeftMargin = yHeight + pLineNew->_yHeight;
                        }

                        //
                        // Update max y of all the left margin's
                        //

                        if (yHeight + pLineNew->_yHeight > _marginInfo._yBottomLeftMargin)
                        {
                            _marginInfo._yBottomLeftMargin = yHeight + pLineNew->_yHeight;
                            if (yHeight + pLineNew->_yHeight > *pyAlignDescent)
                            {
                                *pyAlignDescent = yHeight + pLineNew->_yHeight;
                            }
                        }
                    }
                    else
                    {
                        pLineNew->SetRightAligned();

                        _cRightAlignedLayouts++;

                        pLineNew->_xLineWidth  += pLineNew->_xRight;
                        if(!fRTLDisplay)
                        {
                            if (fMinMaxPass)
                            {
                                pLineNew->_xLeftMargin = _marginInfo._xLeftMargin;
                            }
                            else
                            {
                                pLineNew->_xLeftMargin = max(_marginInfo._xLeftMargin,
                                                         xWidth + 2 * _xMarqueeWidth -                                                     
                                                         (fClearRight ? 0 : _marginInfo._xRightMargin) -
                                                         pLineNew->_xLineWidth);
                                pLineNew->_xLeftMargin = max(_xLayoutLeftIndent,
                                                            pLineNew->_xLeftMargin);
                            }
                        }
                        else
                        {
                            pLineNew->_xRightMargin = fClearRight ? 0 : _marginInfo._xRightMargin;
                        }

                        //
                        // if the current layout has clear then it may not
                        // establish the margin, because it needs to clear
                        // any left aligned layouts if any, in which case
                        // the left margin remains the same.
                        //
                        if (!fClearRight || _marginInfo._xRightMargin == 0)
                        {
                            _marginInfo._xRightMargin += pLineNew->_xLineWidth;
                            _marginInfo._yRightMargin = yHeight + pLineNew->_yHeight;
                        }

                        //
                        // Update max y of all the right margin's
                        //

                        if (yHeight + pLineNew->_yHeight > _marginInfo._yBottomRightMargin)
                        {
                            _marginInfo._yBottomRightMargin = yHeight + pLineNew->_yHeight;
                            if (yHeight + pLineNew->_yHeight > *pyAlignDescent)
                            {
                                *pyAlignDescent = yHeight + pLineNew->_yHeight;
                            }
                        }
                    }
                    
                    if (pxMaxLineWidth)
                    {
                        if(!fRTLDisplay)
                        {
                            *pxMaxLineWidth = max(*pxMaxLineWidth, pLineNew->_xLeftMargin +
                                                                   pLineNew->_xLineWidth);
                        }
                        else
                        {
                            *pxMaxLineWidth = max(*pxMaxLineWidth, pLineNew->_xRightMargin +
                                                                   pLineNew->_xLineWidth);
                        }
                    }

                    if (    _pci->_smMode == SIZEMODE_NATURAL
                        ||  _pci->_smMode == SIZEMODE_SET
                        ||  _pci->_smMode == SIZEMODE_FULLSIZE)
                    {
                        const CFancyFormat * pFF = pNodeLayout->GetFancyFormat();
                        long xPos = 0;

                        Assert(pFF->_bPositionType != stylePositionabsolute);

                        if(!fRTLDisplay)
                            xPos = pLineNew->_xLeftMargin + pLineNew->_xLeft;
                        else
                            xPos -= pLineNew->_xRightMargin + pLineNew->_xRight;

                        if (pFF->_bPositionType != stylePositionrelative)
                        {
                            pme->_pDispNodePrev =
                                _pdp->AddLayoutDispNode(
                                        _pci,
                                        pNodeLayout,
                                        xPos,
                                        yHeight + pLineNew->GetYTop(),
                                        pme->_pDispNodePrev);
                        }
                        else
                        {
                            // Aligned elements have their CSS margin stored in _ptProposed of
                            // their layout.  In the static case above, AddLayoutDispNode()
                            // accounts for it.  This is where we account for it in the
                            // relative case.  (Bug #65664)
                            if(!fRTLDisplay)
                                xPos += pLayout->GetXProposed();
                            else
                            {
                                // we need to set the top left corner.
                                CSize size;
                                pLayout->GetSize(&size);

                                xPos -= (pLayout->GetXProposed() + size.cx);
                            }

                            CPoint  ptAuto(xPos, yHeight + pLineNew->_yBeforeSpace + pLayout->GetYProposed());

                            pNodeLayout->Element()->ZChangeElement(0, &ptAuto);
                        }
                    }

                    // If we are measuring sites at the Beginning of the line. We are
                    // interested only in the current site.
                    if(fMeasuringSitesAtBOL)
                    {
                        break;
                    }
                }
            }
        }
    
        cch -= cchInTreePos;
        ptp = ptp->NextTreePos();
        Assert(ptp);
        cchInTreePos = ptp->GetCch();
    }

Cleanup:
    // Height of the current text line that contains the embedding characters for
    // the aligned sites is added at the end of CRecalcLinePtr::MeasureLine.
    // Here we are taking care of only the clear lines here.
    *pyHeight = yHeight - yHeightCurLine;
    return iClearLines;
}


//+--------------------------------------------------------------------------
//
//  Member:     ClearObjects
//
//  Synopsis:   Insert new line for clearing objects at the left/right margin
//
//  Arguments:  [pLineMeasured]    --  Current line measured.
//              [iLineStart]       --  new lines start at
//              [iLineFirst]       --  line to start calculating from
//              [pyHeight]         --  y coordinate of the top of line
//
//
//---------------------------------------------------------------------------
BOOL CRecalcLinePtr::ClearObjects(
    CLine  *pLineMeasured,
    int     iLineStart,
    int &   iLineFirst,
    LONG   *pyHeight)
{
    CLine * pLine;
    LONG    yAt;
    CLine * pLineNew;

    Reset(iLineStart);
    pLine = (*this)[iLineFirst];


    Assert(_marginInfo._fClearLeft || _marginInfo._fClearRight || _marginInfo._fAutoClear);

    // Find the height to clear
    yAt = *pyHeight;

    if (_marginInfo._fClearLeft && yAt < _marginInfo._yBottomLeftMargin)
    {
        yAt = _marginInfo._yBottomLeftMargin;
    }
    if (_marginInfo._fClearRight && yAt < _marginInfo._yBottomRightMargin)
    {
        yAt = _marginInfo._yBottomRightMargin;
    }
    if (_marginInfo._fAutoClear)
    {
        yAt = max(yAt, min(_marginInfo._yRightMargin, _marginInfo._yLeftMargin));
    }

    if(pLine->_cch > 0 &&
       yAt <= (*pyHeight + pLine->_yHeight))
    {
        // nothing to clear
        pLine->_fClearBefore = pLine->_fClearAfter = FALSE;
    }

    // if the current line has any characters or is an aligned site line,
    // then add a new line, otherwise, re-use the current line.
    else if (pLine->_cch || pLine->IsFrame() && !_marginInfo._fAutoClear)
    {
        if(yAt > *pyHeight)
        {
            BOOL fIsFrame = pLine->IsFrame();
            pLineNew = AddLine();
            pLine = (*this)[iLineFirst];
            if (pLineNew)
            {
                pLineNew->_cch = 0;
                pLineNew->_xLeft = pLine->_xLeft;
                pLineNew->_xRight = pLine->_xRight;
                pLineNew->_xLeftMargin = _marginInfo._xLeftMargin;
                pLineNew->_xRightMargin = _marginInfo._xRightMargin;
                pLineNew->_xLineWidth = max (0L, (_marginInfo._xLeftMargin ? _xMarqueeWidth : 0) +
                                                 (_marginInfo._xRightMargin ? _xMarqueeWidth : 0) +
                                                 _pdp->GetFlowLayout()->GetMaxLineWidth() -
                                                 _marginInfo._xLeftMargin -
                                                 _marginInfo._xRightMargin);
                 
                pLineNew->_xWidth = 0;

                // Make the line the right size to clear.
                if (fIsFrame)
                    pLineNew->_yHeight = pLine->_yHeight;
                else if (pLine->_fForceNewLine)
                {
                    pLineNew->_yHeight = yAt - *pyHeight - pLine->_yHeight;
                }
                else
                {
                    //
                    // if the previous line does not force new line,
                    // then we need to propagate the beforespace.
                    //
                    pLineNew->_yBeforeSpace = pLine->_yBeforeSpace;
                    pLineNew->_yHeight = yAt - *pyHeight;

                    Assert(pLineNew->_yHeight >= pLineNew->_yBeforeSpace);
                }

                *pyHeight += pLineNew->_yHeight;

                pLineNew->_yExtent = pLineNew->_yHeight - pLineNew->_yBeforeSpace;
                pLineNew->_yDescent = 0;
                pLineNew->_yTxtDescent = 0;
                pLineNew->_fForceNewLine = TRUE;
                pLineNew->_fClearAfter = !pLineMeasured->_fClearBefore;
                pLineNew->_fClearBefore = pLineMeasured->_fClearBefore;
                pLineNew->_xLineOverhang = 0;
                pLineNew->_cchWhite = 0;
                pLineNew->_fHasEOP = pLine->_fHasEOP;
                pLine->_fClearBefore = pLine->_fClearAfter = FALSE;
            }
            else
            {
                Assert(FALSE);
                return FALSE;
            }
        }
    }
    // Just replacing the existing empty line.
    else
    {
        pLine->_xWidth = 0;

        // The clear line needs to have a margin for
        // recalc margins to work.
        pLine->_xLeftMargin = _marginInfo._xLeftMargin;
        pLine->_xRightMargin = _marginInfo._xRightMargin;

        pLine->_xLineWidth = max (0L, (_marginInfo._xLeftMargin ? _xMarqueeWidth : 0) +
                                                 (_marginInfo._xRightMargin ? _xMarqueeWidth : 0) +
                                                 _pdp->GetFlowLayout()->GetMaxLineWidth() -
                                                 _marginInfo._xLeftMargin - _marginInfo._xRightMargin);
        pLine->_yHeight = yAt - *pyHeight;
        pLine->_yExtent = pLine->_yHeight;
        pLine->_yDescent = 0;
        pLine->_yTxtDescent = 0;
        pLine->_xLineOverhang = 0;
        pLine->_cchWhite = 0;
        pLine->_fHasBulletOrNum = FALSE;
        pLine->_fClearAfter = !pLineMeasured->_fClearBefore;
        pLine->_fClearBefore = pLineMeasured->_fClearBefore;
        pLine->_fForceNewLine = TRUE;
        if (!_marginInfo._fAutoClear)
            iLineFirst--;

        // If we are clearing the line, then we will come around again to measure
        // the line and we will call CalcBeforeSpace all over again which will
        // add in the padding+border of block elements coming into scope again.
        // To avoid this, remove the padding+border added in this time.
        _xBordLeft  -= _xBordLeftPerLine;
        _xBordRight -= _xBordRightPerLine;
        _xPadLeft   -= _xPadLeftPerLine;
        _xPadRight  -= _xPadRightPerLine;
    }
    if(pLine->_fForceNewLine)
        *pyHeight += pLine->_yHeight;

    // Prepare for the next pass of the measurer.
    _marginInfo._fClearLeft  = FALSE;
    _marginInfo._fClearRight = FALSE;
    
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::MeasureLine()
//
//  Synopsis:   Measures a new line, creates aligned and clear lines if
//              current text line has aligned objects or clear flags set on it.
//              Updates iNewLine to point to the last text line. -1 if there is
//              no text line before the current line
//
//-----------------------------------------------------------------------------

BOOL
CRecalcLinePtr::MeasureLine(CLSMeasurer &me,
                            UINT    uiFlags,
                            INT  *  piLine,
                            LONG *  pyHeight,
                            LONG *  pyAlignDescent,
                            LONG *  pxMinLineWidth,
                            LONG *  pxMaxLineWidth)
{
    LONG                cchSkip = 0;
    LONG                xWidth = 0;
    CLine              *pliNew;
    INT                 iTLine = *piLine - 1;
    BOOL                fAdjustForCompact = FALSE;
    CFlowLayout        *pFlowLayout = _pdp->GetFlowLayout();
    BOOL                fClearMarginsRequired;
    CTreeNode          *pNodeFormatting;

    Reset(_iNewFirst);
    pliNew = (*this)[*piLine];

    _xLeadAdjust = 0;

    me._cchAbsPosdPreChars = 0;
    me._fRelativePreChars = FALSE;

    _marginInfo._fClearLeft  =
    _marginInfo._fClearRight =
    _marginInfo._fAutoClear  = FALSE;

    // If the current line being measure has an offset of 0, It implies
    // the current line is the top most line.
    if (*pyHeight == 0 )
        uiFlags |= MEASURE_FIRSTLINE;

#if DBG==1
    // We should never have an index off the end of the line array.
    if (iTLine >= 0 && (*this)[iTLine] == NULL)
    {
        Assert(FALSE);
        goto err;
    }
#endif

    // If we are measuring aligned sites at the beginnning of the line,
    // we have initialized the line once.
    me.NewLine(uiFlags & MEASURE_FIRSTINPARA);

    // Space between paragraphs.
    pNodeFormatting = CalcInterParaSpace (&me, iTLine, uiFlags & MEASURE_FIRSTLINE ? TRUE : FALSE);
    if (me._li._fFirstInPara)
        uiFlags |= MEASURE_FIRSTINPARA;

    //
    // Note: CalcBeforeSpace needs to be call ed before we call RecalcMargins
    // because before space is used to compute margins.
    //
    
    // If the current line being measured has invalid margins, a line that is
    // below an aligned line, margins have changed so recalculate margins.
    if (!IsValidMargins(*pyHeight + max(SHORT(0), me._li._yBeforeSpace)))
    {
        BOOL fClearLeft  = _marginInfo._fClearLeft;
        BOOL fClearRight = _marginInfo._fClearRight;

        RecalcMargins(_iNewFirst, *piLine, *pyHeight, me._li._yBeforeSpace);

        if (_fMarginFromStyle)
            uiFlags |= MEASURE_AUTOCLEAR;

        //
        // Restore the clear flags on _marginInfo, since RecalcMargins init's
        // the marginInfo. CalcBeforeSpace set's up clear flags on the marginInfo
        // if any elements comming into scope have clear attribute/style set.
        //
        _marginInfo._fClearLeft  = fClearLeft;
        _marginInfo._fClearRight = fClearRight;
    }

    //
    // If we need to clear aligned elements, don't bother measuring text
    //
    if (!CheckForClear())
    {
        // Apply the line indents, _xLeft and _xRight
        ApplyLineIndents(pNodeFormatting, &me._li, uiFlags);
        
        if( iTLine >= 0 )
        {
            BOOL  fInner = FALSE;
            const CParaFormat* pPF;

            pPF = me.MeasureGetPF(&fInner);

            CLine *pli = (*this)[iTLine];

            // If we have the compact attribute set, see if we can fit the DT and the DD on the
            //  same line.
            if( pPF->HasCompactDL(fInner) && pli->_fForceNewLine )
            {
                CTreeNode * pNodeCurrBlockElem = pNodeFormatting;

                if (pNodeCurrBlockElem->Element()->IsTagAndBlock(ETAG_DD))
                {
                    CTreePos  * ptp;
                    CTreePos  * ptpFirst;
                    CTreeNode * pNodePrevBlockElem;

                    //
                    // Find the DT which should have appeared before the DD
                    //
                    // Get the max we will travel backwards in search of the DT
                    pFlowLayout->GetContentTreeExtent(&ptpFirst, NULL);

                    pNodePrevBlockElem = NULL;
                    ptp = me.GetPtp();
                    for(;;)
                    {
                        ptp = ptp->PreviousTreePos();
                        Assert(ptp);
                        if (   (ptp == ptpFirst)
                            || ptp->IsText()
                           )
                            break;

                        if (   ptp->IsEndElementScope()
                            && ptp->Branch()->Element()->IsTagAndBlock(ETAG_DT)
                           )
                        {
                            pNodePrevBlockElem = ptp->Branch();
                            break;
                        }
                    }

#if DBG==1                
                    if (pNodePrevBlockElem)
                    {
                        Assert(pNodePrevBlockElem->Element()->IsTagAndBlock(ETAG_DT));
                        Assert((pFlowLayout->GetContentMarkup()->FindMyListContainer(pNodeCurrBlockElem) ==
                                pFlowLayout->GetContentMarkup()->FindMyListContainer(pNodePrevBlockElem)));
                    }
#endif
                    // Make sure that the DT is thin enough to fit the DD on the same line.
                    if (   pNodePrevBlockElem
                        && (!pPF->_fRTL ? me._li._xLeft > pli->_xWidth + pli->_xLeft + pli->_xLineOverhang
                                        : me._li._xRight > pli->_xWidth + pli->_xRight + pli->_xLineOverhang)
                       )
                    {
                        fAdjustForCompact = TRUE;
                        pli->_fForceNewLine = FALSE;
                        if(!pPF->_fRTL)
                        {
                            pli->_xRight = 0;
                            pli->_xLineWidth = me._li._xLeft;
                        }
                        else
                        {
                            pli->_xLeft = 0;
                            pli->_xLineWidth = me._li._xRight;
                        }
                        *pyHeight -= pli->_yHeight;

                        // If the current line being measured has an offset of 0, It implies
                        // the current line is the top most line.
                        if (*pyHeight == 0 )
                            uiFlags |= MEASURE_FIRSTLINE;

                        // Adjust the margins
                        RecalcMargins(_iNewFirst, *piLine, *pyHeight, me._li._yBeforeSpace);
                        if (_fMarginFromStyle)
                            uiFlags |= MEASURE_AUTOCLEAR;
                    }
                }
            }
        }

        me._cyTopBordPad = _yBordTop + _yPadTop;

        // Compute the available width for the current line.
        // (Do this after determining the margins for the line)
        xWidth = GetAvailableWidth();

        cchSkip = CalcAlignedSitesAtBOL(&me, &me._li, uiFlags,
                                        piLine, pyHeight,
                                        &xWidth, pyAlignDescent,
                                        pxMaxLineWidth, &fClearMarginsRequired);

        pliNew = (*this)[*piLine];
        if (!fClearMarginsRequired)
        {
            me._cchPreChars = me._li._cch;
            me._li._cch = 0;

            if (me._fMeasureFromTheStart)
                me.SetCp(me.GetCp() - me._cchPreChars, NULL);
            
            if (_fMarginFromStyle)
                uiFlags |= MEASURE_AUTOCLEAR;

            me._pLS->_cchSkipAtBOL = cchSkip;
            me._yli = *pyHeight;
            if(!me.MeasureLine(xWidth, -1, uiFlags, &_marginInfo, pxMinLineWidth))
            {
                Assert(FALSE);
                goto err;
            }

            Assert(cchSkip <= me._li._cch);
            
            // if it is in edit mode _fNoContent will be always FALSE
            // If it has a LI on it then too we will say that the display has contents
            _pdp->_fNoContent =    !_fIsEditable
                                && !!(uiFlags & MEASURE_FIRSTLINE && me._li._cch == 0)
                                && !me._li._fHasBulletOrNum;

            me.Resync();
            if (!me._fMeasureFromTheStart)
                me._li._cch += me._cchPreChars;

            if (me._pLS->_cInlinedSites + me._pLS->_cAlignedSites)
            {
                _pdp->_fHasEmbeddedLayouts = TRUE;
            }

            // If we couldn't fit anything on the line, clear an aligned thing and
            // we'll try again afterwards.
            if (!_marginInfo._fAutoClear || me._li._cch > 0)
            {
                // CalcAfterSpace modifies, _xBordLeft and _xBordRight
                // to account for block elements going out of scope,
                // set the flag before they are modified.
                me._li._fHasParaBorder = (_xBordLeft + _xBordRight) != 0;

                CalcAfterSpace(&me, LONG_MAX);

                // Monitor progress here
                Assert(me._li._cch != 0 || _pdp->GetLastCp() == me.GetCp());

                me._li._yHeight  += _yBordTop + _yBordBottom + 
                                    _yPadTop  + _yPadBottom;
                me._li._yDescent += _yBordBottom + _yPadBottom;
                me._li._yHeight  += (LONG)me._li._yBeforeSpace;
                me._li._yExtent  += _yBordTop + _yBordBottom + 
                                    _yPadTop  + _yPadBottom;
                me._li._fHasParaBorder |= (_yBordTop + _yBordBottom) != 0;

                _yBordTop = _yBordBottom = _yPadTop = _yPadBottom = 0;

                AssertSz(!IsBadWritePtr(pliNew, sizeof(CLine)),
                         "Line Array has been realloc'd and now pliNew is invalid! "
                         "Memory corruption is about to occur!");

                // This innocent looking line is where we transfer
                // the calculated values to the permanent line array.
                *pliNew = me._li;

                // Remember the longest word.
                if(pxMinLineWidth && _pdp->_xMinWidth >= 0)
                {
                    _pdp->_xMinWidth = max(_pdp->_xMinWidth, *pxMinLineWidth);
                }
            }
            else
            {
                *pliNew = me._li;
                ResetPosAndNegSpace();
            }
        }
        else
        {
            *pliNew = me._li;
            me.Advance(cchSkip);

            ResetPosAndNegSpace();

            pliNew->_cch            += cchSkip;
            pliNew->_fHasEmbedOrWbr = TRUE;
            pliNew->_fDummyLine     = TRUE;
            pliNew->_fForceNewLine  = FALSE;
            pliNew->_fClearAfter    = TRUE;
        }
    }
    else
    {
        pliNew->_fClearBefore = TRUE;
        pliNew->_xWhite =
        pliNew->_xWidth = 0;
        pliNew->_cch =
        pliNew->_cchWhite = 0;
        pliNew->_yBeforeSpace = me._li._yBeforeSpace;
        me.Advance(-me._li._cch);
        me._li._cch = 0;
    }

    // If we're autoclearing, we don't need to do anything here.
    if (!_marginInfo._fAutoClear || me._li._cch > 0)
    {
        if( fAdjustForCompact )
        {
            CLine *pli = (*this)[iTLine];       // since fAdjustForCompact is set, we know iTLine >= 0

            // The DT and DL have different heights, need to make them each the height of the
            //  greater of the two.

            SHORT iDTHeight = pli->_yHeight - pli->_yBeforeSpace - pli->_yDescent;
            SHORT iDDHeight = pliNew->_yHeight - pliNew->_yBeforeSpace - pliNew->_yDescent;

            pli->_yBeforeSpace    += (SHORT)max( 0, iDDHeight - iDTHeight );
            pliNew->_yBeforeSpace += (SHORT)max( 0, iDTHeight - iDDHeight );
            pli->_yDescent        += (SHORT)max( 0, pliNew->_yDescent - pli->_yDescent );
            pliNew->_yDescent     += (SHORT)max( 0, pli->_yDescent - pliNew->_yDescent );

            pli->_yHeight = pliNew->_yHeight =
                pli->_yBeforeSpace +  iDTHeight + pli->_yDescent;
        }

        pliNew->_xLeftMargin = _marginInfo._xLeftMargin;
        pliNew->_xRightMargin = _marginInfo._xRightMargin;
        pliNew->_xLineWidth  = pliNew->_xLeft         +
                               pliNew->_xWidth        +
                               pliNew->_xLineOverhang +
                               pliNew->_xRight;
        if(pliNew->_fForceNewLine && xWidth > pliNew->_xLineWidth)
            pliNew->_xLineWidth = xWidth;
        
        if(pliNew->_fHidden)
        {
            pliNew->_yExtent = 0;
        }
        
        if(pxMaxLineWidth)
        {
            *pxMaxLineWidth = max(*pxMaxLineWidth, (!pliNew->_fRTL ? pliNew->_xLeftMargin : pliNew->_xRightMargin)   +
                                                   pliNew->_xWidth        +
                                                   pliNew->_xLeft         +
                                                   pliNew->_xLineOverhang +
                                                   pliNew->_xRight);
        }

        // Align those sites which occur somewhere in the middle or at end of the line
        // (Those occurring at the start are handled in the above loop)
        if(pliNew->_fHasAligned &&
           me._cAlignedSites > me._cAlignedSitesAtBeginningOfLine)
        {
            // Position the measurer so that it points to the beginning of the line
            // excluding all the whitespace characters.
            LONG cch = pliNew->_cch - me.CchSkipAtBeginningOfLine();

            CTreePos *ptpOld = me.GetPtp();
            LONG cpOld = me.GetCp();
            
            me.Advance(-cch);

            AlignObjects(&me, &me._li, cch, FALSE,
                                   (uiFlags & MEASURE_BREAKATWORD) ? TRUE : FALSE,
                                   pxMinLineWidth ? TRUE : FALSE,
                                   _iNewFirst, *piLine, pyHeight, xWidth,
                                   pyAlignDescent, pxMaxLineWidth);

            me.SetPtp(ptpOld, cpOld);
            
            pliNew = (*this)[*piLine];
        }

        // NETSCAPE: Right-aligned sites are not normally counted in the maximum line width.
        //           However, the maximum line width should not be allowed to shrink below
        //           the minimum (which does include all left/right-aligned sites guaranteed
        //           to occur on a single line).
        if (pxMaxLineWidth && pxMinLineWidth)
        {
            *pxMaxLineWidth = max(*pxMaxLineWidth, *pxMinLineWidth);
        }

        if(me._pLS->HasChunks())
        {
            FixupChunks(me, piLine);
            pliNew = (*this)[*piLine];
        }
    }

    if (_marginInfo._fClearLeft || _marginInfo._fClearRight || _marginInfo._fAutoClear)
    {
        ClearObjects(&me._li, _iNewFirst, *piLine, pyHeight);

        // The calling function(s) expect to point to the last text line.
        while(*piLine >= 0)
        {
            pliNew = (*this)[*piLine];
            if(pliNew->IsTextLine())
                break;
            else
                (*piLine)--;
        }
    }
    // Sometimes we don't have a line at this point, stress bug. arye
    else if(pliNew && pliNew->_fForceNewLine)
    {
        *pyHeight += pliNew->_yHeight;
    }
    return TRUE;

err:
    return FALSE;
}

                                        
//+----------------------------------------------------------------------------
//
//  Member:   CRecalcLinePtr::CalcAlignedSitesAtBOL()
//
//  Synopsis: Measures any aligned sites which are at the BOL.
//
//  Params:
//    prtp(i,o):          The position in the runs for the text
//    pLineMeasured(i,o): The line being measured
//    uiFlags(i):         The flags controlling the measurer behaviour
//    piLine(i,o):        The line before which all aligned lines are added.
//                           Incremented to reflect addition of lines.
//    pxWidth(i,o):       Contains the available width for the line. Aligned
//                           lines will decrease the available width.
//    pxMinLineWidth(o):  These two are passed directly to AlignObjects.
//    pxMaxLineWidth(o):
//
//  Return:     LONG    -   the no of character's to skip at the beginning of
//                          the line
//
//-----------------------------------------------------------------------------
LONG
CRecalcLinePtr::CalcAlignedSitesAtBOL(
      CLSMeasurer * pme,
      CLine       * pLineMeasured,
      UINT          uiFlags,
      INT         * piLine,
      LONG        * pyHeight,
      LONG        * pxWidth,
      LONG        * pyAlignDescent,
      LONG        * pxMaxLineWidth,
      BOOL        * pfClearMarginsRequired)
{
    CTreePos *ptp;

    ptp = pme->GetPtp();
    if (ptp->IsText() && ptp->Cch())
    {
        TCHAR ch = CTxtPtr(_pdp->GetMarkup(), pme->GetCp()).GetChar();
        CTreeNode *pNode = pme->CurrBranch();

        // Check to see whether this line begins with whitespace;
        // if it doesn't, or if it does but we're in a state where whitespace is significant
        // (e.g. inside a PRE tag), then by definition there are no aligned
        // sites at BOL (because there's something else there first).
        // Similar code is in CalcAlignedSitesAtBOLCore() for handling text
        // between aligned elements.

        // BUGBUG: We may want to do a loop for all cch and handle
        // each one (see code in the Core version)

        if ( !IsWhite(ch) || pNode->GetParaFormat()->HasInclEOLWhite( SameScope(pNode, pme->_pFlowLayout->ElementContent()) ) )
        {
            Assert(_fIsEditable || !pme->CurrBranch()->GetCharFormat()->IsDisplayNone());
            
            *pfClearMarginsRequired = FALSE;
            return 0;
        }
    }
    
    return CalcAlignedSitesAtBOLCore(pme, pLineMeasured, uiFlags, piLine,
                                     pyHeight, pxWidth, pyAlignDescent,
                                     pxMaxLineWidth, pfClearMarginsRequired);
}

LONG
CRecalcLinePtr::CalcAlignedSitesAtBOLCore(
        CLSMeasurer *pme,
        CLine       *pLineMeasured,
        UINT         uiFlags,
        INT         *piLine,
        LONG        *pyHeight,
        LONG        *pxWidth,
        LONG        *pyAlignDescent,
        LONG        *pxMaxLineWidth,
        BOOL        *pfClearMarginsRequired)
{
    CFlowLayout *pFlowLayout = _pdp->GetFlowLayout();
    CElement    *pElementFL  = pFlowLayout->ElementContent();
    const        CCharFormat *pCF;              // The char format
    LONG         cpSave   = pme->GetCp();       // Saves the cp the measurer is at
    CTreePos    *ptpSave  = pme->GetPtp();
    BOOL         fAnyAlignedSiteMeasured;       // Did we measure any aligned site at all?
    CTreePos    *ptpLayoutLast;
    CTreePos    *ptp;
    LONG         cp;
    CTreeNode   *pNode;
    CElement    *pElement;
    BOOL         fInner;
    
    AssertSz(!(uiFlags & MEASURE_BREAKWORDS),
             "Cannot call CalcSitesAtBOL when we are hit testing!");

    //
    // By default, do not clear margins
    //
    *pfClearMarginsRequired = FALSE;
    fAnyAlignedSiteMeasured = FALSE;

    pFlowLayout->GetContentTreeExtent(NULL, &ptpLayoutLast);

    ptp = ptpSave;
    cp  = cpSave;
    pNode = ptp->GetBranch();
    pElement = pNode->Element();
    pCF = pNode->GetCharFormat();
    fInner = SameScope(pNode, pElementFL);
            
    for (;;)
    {
        pme->SetPtp(ptp, cp);

        if (ptp == ptpLayoutLast)
            break;

        if (ptp->IsPointer())
        {
            ptp = ptp->NextTreePos();
            continue;
        }

        if (ptp->IsNode())
        {
            pNode = ptp->Branch();
            pElement = pNode->Element();
            fInner = SameScope(pNode, pElementFL);
            if (ptp->IsEndNode())
                pNode = pNode->Parent();
            pCF = pNode->GetCharFormat();
        }

        //
        // NOTE(SujalP):
        // pCF should never be NULL since though it starts out as NULL, it should
        // be initialized the first time around in the code above. It has happened
        // in stress once that pCF was NULL. Hence the check for pCF and the assert.
        // (Bug 18065).
        //
        AssertSz(pNode && pElement && pCF, "None of these should be NULL");
        if (!(pNode && pElement && pCF))
            break;

        if (   !_fIsEditable
            && ptp->IsBeginElementScope()
            && pCF->IsDisplayNone()
           )
        {
            cp += pme->GetNestedElementCch(pNode->Element(), &ptp) - 1;
        }
        else if (   ptp->IsBeginElementScope()
                 && pNode->NeedsLayout()
                )
        {
            pLineMeasured->_fHasNestedRunOwner |= pNode->Element()->IsRunOwner();
            
            if (!pElement->IsInlinedElement())
            {
                //
                // Absolutely positioned sites are measured separately
                // inside the measurer. They also count as whitespace and
                // hence we will continue looking for aligned sites after
                // them at BOL.
                //
                if (!pNode->IsAbsolute())
                {
                    CLine *pLine;

                    //
                    // Mark the line which will contain the WCH_EMBEDDING as having
                    // an aligned site in it.
                    //
                    pLine = (*this)[*piLine];
                    pLine->_fHasAligned = TRUE;

                    //
                    // Measure the aligned site and create a line for it.
                    // AlignObjects returns the number of clear lines, so we need
                    // to add the number of clear lines + the aligned line that is
                    // inserted to *piLine to make *piLine point to the text line
                    // that contains the embedding characters for the aligned site.
                    //
                    *piLine += AlignObjects(pme, pLineMeasured, 1, TRUE,
                                            (uiFlags & MEASURE_BREAKATWORD) ? TRUE : FALSE,
                                            (uiFlags & MEASURE_MAXWIDTH)    ? TRUE : FALSE,
                                            _iNewFirst, *piLine, pyHeight, *pxWidth,
                                            pyAlignDescent, pxMaxLineWidth);

                    //
                    // This the now the next available line, because we just inserted
                    // a line for the aligned site.
                    //
                    (*piLine)++;

                    //
                    // Aligned objects change the available width for the line
                    //
                    *pxWidth = GetAvailableWidth();

                    //
                    // BUGBUG(SriniB) What's this for really?
                    //
                    _xLeadAdjust = 0;

                    //
                    // Also update the xLeft and xRight for the next line
                    // to be inserted.
                    //
                    ApplyLineIndents(pNode, pLineMeasured, uiFlags);
                    
                    //
                    // Remember we measured an aligned site in this pass
                    //
                    fAnyAlignedSiteMeasured = TRUE;
                }

                cp += pme->GetNestedElementCch(pElement, &ptp) - 1;
            }
            else
            {
                if (CheckForClear(pNode))
                {
                    *pfClearMarginsRequired = TRUE;
                }
                break;
            }
        }
        else if (ptp->IsText())
        {
            CTxtPtr tp(_pdp->GetMarkup(), cp);
            // NOTE: Cch() could return 0 here but we should be OK with that.
            LONG cch = ptp->Cch();
            BOOL fNonWhitePresent = FALSE;
            TCHAR ch;
            
            while (cch)
            {
                ch = tp.GetChar();
                //
                // These characters need to be treated like whitespace
                //
                if (!(   ch == _T(' ')
                      || (   InRange(ch, TEXT('\t'), TEXT('\r'))
                          && !pNode->GetParaFormat()->HasInclEOLWhite(
                                SameScope(pNode, pme->_pFlowLayout->ElementContent()))
                         )
                       )
                   )
                {
                    fNonWhitePresent = TRUE;
                    break;
                }
                cch--;
                tp.AdvanceCp(1);
            }
            if (fNonWhitePresent)
                break;
        }
        else if (   ptp->IsEdgeScope()
                 && pElement != pElementFL
                 && (   pFlowLayout->IsElementBlockInContext(pElement)
                     || pElement->Tag() == ETAG_BR
                    )
                )
        {
            break;
        }
        else if ( ptp->IsBeginElementScope() && CheckForClear(pNode) )
        {
            *pfClearMarginsRequired = TRUE;
            break;
        }
        
        //
        // Goto the next tree position
        //
        cp += ptp->GetCch();
        ptp = ptp->NextTreePos();
    }

    //
    // Restore the measurer to where it was when we came in
    //
    pme->SetPtp(ptpSave, cpSave);

    return fAnyAlignedSiteMeasured ? cp - cpSave : 0;
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::CheckForClear()
//
//  Synopsis:   CalcBeforeSpace set's up _fClearLeft & _fClearRight on the
//              _marginInfo, if a pNode is passed the use its clear flags,
//              check if we need to clear based on margins.
//
//  Arguments:  pNode - (optional) can be null.
//
//  Returns: A bool indicating if clear is required.
//
//-----------------------------------------------------------------------------
BOOL
CRecalcLinePtr::CheckForClear(CTreeNode * pNode)
{
    BOOL fClearLeft;
    BOOL fClearRight;

    if (pNode)
    {
        const CFancyFormat * pFF = pNode->GetFancyFormat();

        fClearLeft  = pFF->_fClearLeft;
        fClearRight = pFF->_fClearRight;
    }
    else
    {
        fClearLeft  = _marginInfo._fClearLeft;
        fClearRight = _marginInfo._fClearRight;
    }

    _marginInfo._fClearLeft  = _marginInfo._xLeftMargin && fClearLeft;
    _marginInfo._fClearRight = _marginInfo._xRightMargin && fClearRight;

    return _marginInfo._fClearLeft || _marginInfo._fClearRight;
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::FixupChunks
//
//  Synopsis:   If the current line has multiple chunks in the line (caused by
//              relative chunks), then break the line into multiple lines that
//              form a single screen line and fix up justification.
//
//  REMINDER:   RightToLeft displays measure from the right. Therefore, the
//              chunks need to be handled accordingly.
//-----------------------------------------------------------------------------

void
CRecalcLinePtr::FixupChunks(CLSMeasurer &me, INT *piLine)
{
    CLSLineChunk * plc  = me._pLS->GetFirstChunk();
    BOOL    fRTLDisplay = _pdp->IsRTL();
    CLine * pliT        = (*this)[*piLine];
    CLine   li          = *pliT;
    LONG    cchLeft     = li._cch;
    LONG    xWidthLeft  = li._xWidth;
    LONG    xWidth      = !fRTLDisplay ? li._xLeft : li._xRight;
    BOOL    fFirstLine  = TRUE;
    LONG    iFirstChunk = *piLine;

    // if all we have is one chunk in the line, we dont need to create
    // additional lines
    if(plc->_cch >= cchLeft)
    {
        pliT->_fRelative            = me._pLS->_pElementLastRelative != NULL;
        pliT->_fPartOfRelChunk      = TRUE;
        _pdp->NoteMost(pliT);
        return;
    }

    while(plc || cchLeft)
    {
        BOOL fLastLine = !plc || cchLeft <= plc->_cch;
        BOOL fMoveBulletOrNum;

        if(!fFirstLine)
        {
            pliT = InsertLine(++(*piLine));

            if (!pliT)
                break;
            
            *pliT = li;

            if(!fRTLDisplay)
                pliT->_xLeft        = xWidth;
            else
                pliT->_xRight       = xWidth;

            pliT->_fHasBulletOrNum  = FALSE;
            pliT->_fFirstFragInLine = FALSE;
        }

        pliT->_cch                  = min(cchLeft, long(plc ? plc->_cch : cchLeft));
        pliT->_xWidth               = min(long(plc ? plc->_xWidth : xWidthLeft), xWidthLeft);
        pliT->_fRelative            = plc ? plc->_fRelative : me._pLS->_pElementLastRelative != NULL;
        pliT->_fSingleSite          = plc ? plc->_fSingleSite : me._pLS->_fLastChunkSingleSite;
        pliT->_fPartOfRelChunk      = TRUE;
        pliT->_fHasEmbedOrWbr       = li._fHasEmbedOrWbr;

        if(fLastLine)
        {
            if(!fRTLDisplay)
                pliT->_xRight       = li._xRight;
            else
                pliT->_xLeft        = li._xLeft;

            pliT->_cchWhite         = min(pliT->_cch, long(li._cchWhite));
            pliT->_xWhite           = min(pliT->_xWidth, long(li._xWhite));
            pliT->_xLineOverhang    = li._xLineOverhang;
            pliT->_fClearBefore     = li._fClearBefore;
            pliT->_fClearAfter      = li._fClearAfter;
            pliT->_fForceNewLine    = li._fForceNewLine;
            pliT->_fHasEOP          = li._fHasEOP;
            pliT->_fHasBreak        = li._fHasBreak;
            pliT->_xLineWidth       = li._xLineWidth;

            fMoveBulletOrNum        = me._pLS->_fLastChunkHasBulletOrNum;
        }
        else
        {
            if(!fRTLDisplay)
                pliT->_xRight       = 0;
            else
                pliT->_xLeft        = 0;
            pliT->_cchWhite         = 0;
            pliT->_xWhite           = 0;
            pliT->_xLineOverhang    = 0;
            pliT->_fClearBefore     = FALSE;
            pliT->_fClearAfter      = FALSE;
            pliT->_fForceNewLine    = FALSE;
            pliT->_fHasEOP          = FALSE;
            pliT->_fHasBreak        = FALSE;

            pliT->_xLineWidth       = pliT->_xLeft + pliT->_xWidth +
                                        pliT->_xLineOverhang + pliT->_xRight;

#ifdef NEVER
            if(long(me.GetCp()) == me.GetLastCp() &&
                !(xWidthLeft - pliT->_xWidth) && cchLeft && pliT->_xWidth)
            {
                pliT->_xLineWidth += li._xLineWidth - li._xWidth - li._xLineOverhang -
                                        li._xLeft - li._xRight;
            }
#endif

            fMoveBulletOrNum = plc && plc->_fHasBulletOrNum;
        }

        if (fMoveBulletOrNum && !fFirstLine)
        {
            CLine * pli0 = (*this)[iFirstChunk];

            pliT->_fHasBulletOrNum  = pli0->_fHasBulletOrNum;
            pli0->_fHasBulletOrNum  = FALSE;
        }

        fFirstLine  =  FALSE;
        plc         =  plc ? plc->_plcNext : NULL;
        xWidth      =  pliT->_xLineWidth;
        xWidthLeft  -= pliT->_xWidth;
        cchLeft     -= pliT->_cch;

        _pdp->NoteMost(pliT);
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::CalcParagraphSpacing
//
//  Synopsis:   Compute paragraph spacing for the current line
//
//-----------------------------------------------------------------------------
CTreeNode *
CRecalcLinePtr::CalcParagraphSpacing(
    CLSMeasurer *pMe,
    BOOL         fFirstLineInLayout)
{
    CTreePos *ptp = pMe->GetPtp();
    CTreeNode *pNode;
    
    Assert(   !ptp->IsBeginNode()
           || !_pdp->GetFlowLayout()->IsElementBlockInContext(ptp->Branch()->Element())
           || !ptp->Branch()->Element()->IsInlinedElement()
           || pMe->_li._fFirstInPara
          );
    
    // no bullet on the line
    pMe->_li._fHasBulletOrNum = FALSE;

    // Reset this flag for every line that we measure.
    _fNoMarginAtBottom = FALSE;

    // Only interesting for the first line of a paragraph.
    if (pMe->_li._fFirstInPara || pMe->_fLastWasBreak)
    {
        ptp = CalcBeforeSpace(pMe, fFirstLineInLayout);
        pMe->_li._yBeforeSpace = _lTopPadding + _lNegSpace + _lPosSpace;
    }
    // Not at the beginning of a paragraph, we have no interline spacing.
    else
    {
        pMe->_li._yBeforeSpace = 0;
    }

    pNode = ptp->GetBranch();
    pMe->MeasureSetPF(pNode->GetParaFormat(), SameScope(pNode, _pdp->GetFlowLayout()->ElementContent()));
    
    return pNode; // formatting node
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::SetupMeasurerForBeforeSpace
//
//  Synopsis:   Setup the measurer so that it has all the post space info collected
//              from the previous line. This function is called only ONCE when
//              per recalclines loop. Subsequenlty we keep then spacing in sync
//              as we are measuring the lines.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

void
CRecalcLinePtr::SetupMeasurerForBeforeSpace(CLSMeasurer *pMe)
{
    CFlowLayout  *pFlowLayout = _pdp->GetFlowLayout();
    CElement     *pElementFL  = pFlowLayout->ElementContent();
    LONG          cpSave      = pMe->GetCp();
    CTreePos     *ptp;
    CTreeNode    *pNode;

    INSYNC(pMe);

    ResetPosAndNegSpace();
    
    _pdp->EndNodeForLine(pMe->GetCp(), pMe->GetPtp(), NULL, &ptp, &pMe->_pLS->_pNodeForAfterSpace);

    if (   ptp != pMe->GetPtp()
        || pMe->_pLS->_pNodeForAfterSpace
       )
    {
        pMe->SetPtp(ptp, -1);
    
        //
        // Having gone back to a point where there is some text or a layout(which
        // effectively means some text) we compute the "after" space. The point
        // which we go back to is effectively the point at which we would have
        // stopped measuring the previous line.
        //
        CalcAfterSpace(pMe, cpSave);
    }
    
    //
    // Be sure that calc after space gets us at the beginning of the
    // current line. If it leaves us before the current line then there is a
    // _VERY_ good chance that we will end up with more characters in the line
    // array than there are in the backing story.
    //
    Assert(pMe->GetCp() == cpSave);

    // initialize left and right padding & borderspace for parent block elements
    _xPadLeft = _xPadRight = _xBordLeft = _xBordRight = 0;
    _yPadTop = _yPadBottom = _yBordTop = _yBordBottom = 0;
    _xBordLeftPerLine = _xBordRightPerLine = _xPadLeftPerLine = _xPadRightPerLine = 0;

    pNode     = pMe->CurrBranch();

    // Measurer can be initialized for any line in the line array, so
    // compute the border and padding for all the block elements that
    // are currently in scope.
    if(DifferentScope(pNode, pElementFL))
    {
        CDoc      * pDoc = pFlowLayout->Doc();
        CTreeNode * pNodeTemp;
        CElement  * pElement;

        pNodeTemp = pMe->GetPtp()->IsBeginElementScope()
                        ? pNode->Parent()
                        : pNode;

        while(DifferentScope(pNodeTemp, pElementFL))
        {
            pElement = pNodeTemp->Element();

            if(     !pNodeTemp->NeedsLayout()
                &&   pNodeTemp->GetParaFormat()->HasPadBord(FALSE)
                &&   pFlowLayout->IsElementBlockInContext(pElement))
            {
                const CFancyFormat * pFF = pNodeTemp->GetFancyFormat();
                LONG lFontHeight = pNodeTemp->GetCharFormat()->GetHeightInTwips(pDoc);

                if ( !pElement->_fDefinitelyNoBorders )
                {
                    CBorderInfo borderinfo;

                    pElement->_fDefinitelyNoBorders =
                        !GetBorderInfoHelper( pNodeTemp, _pci, &borderinfo, FALSE );
                    if ( !pElement->_fDefinitelyNoBorders )
                    {
                        _xBordLeftPerLine  += borderinfo.aiWidths[BORDER_LEFT];
                        _xBordRightPerLine += borderinfo.aiWidths[BORDER_RIGHT];
                    }
                }

                _xPadLeftPerLine  += pFF->_cuvPaddingLeft.XGetPixelValue(
                                        _pci,
                                        _pci->_sizeParent.cx, 
                                        lFontHeight);
                _xPadRightPerLine += pFF->_cuvPaddingRight.XGetPixelValue(
                                        _pci,
                                        _pci->_sizeParent.cx, 
                                        lFontHeight);
            }
            pNodeTemp = pNodeTemp->Parent();
        }

        _xBordLeft  = _xBordLeftPerLine;
        _xBordRight = _xBordRightPerLine;
        _xPadLeft   = _xPadLeftPerLine;
        _xPadRight  = _xPadRightPerLine;
    }

    INSYNC(pMe);
    return;
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::CalcAfterSpace
//
//  Synopsis:   This function computes the after space of the line and adds
//              on the extra characters at the end of the line. Also positions
//              the measurer correctly (to the ptp at the ptp belonging to
//              the first character in the next line).
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

void
CRecalcLinePtr::CalcAfterSpace(CLSMeasurer *pMe, LONG cpMax)
{
    CFlowLayout  *pFlowLayout = _pdp->GetFlowLayout();
    CTreeNode    *pNode;
    CElement     *pElement;
    CTreePos     *ptpStop;
    CTreePos     *ptp;
    CUnitValue    cuv;
    LONG          cpCurrent;
    BOOL          fConsumedFirstOne = pMe->_fEndSplayNotMeasured;
    
    INSYNC(pMe);
    
    if (pMe->_li._fForceNewLine)
    {
        ResetPosAndNegSpace();
    }

    if (pMe->_pLS->_pNodeForAfterSpace)
    {
        CollectSpaceInfoFromEndNode(pMe->_pLS->_pNodeForAfterSpace, FALSE);
    }
    
    // initialize bottom padding & border
    _yPadBottom = _yBordBottom = 0;

    ptpStop = pMe->_pLS->_treeInfo._ptpLayoutLast;
#if DBG==1
    {
        CTreePos *ptpStartDbg;
        CTreePos *ptpStopDbg;
        pFlowLayout->GetContentTreeExtent(&ptpStartDbg, &ptpStopDbg);
        Assert(ptpStop == ptpStopDbg);
    }
#endif
    
    for (cpCurrent = pMe->GetCp(), ptp = pMe->GetPtp();
         cpCurrent < cpMax;
         ptp = ptp->NextTreePos())
    {
        if (ptp->IsPointer())
            continue;

        if (ptp == ptpStop)
            break;

        Assert(cpCurrent >= ptp->GetCp() && cpCurrent < ptp->GetCp() + ptp->GetCch());
        
        if (ptp->IsNode())
        {
            pNode = ptp->Branch();
            pElement = pNode->Element();
            
            if (ptp->IsEndElementScope())
            {
                //
                // NOTE(SujalP):
                // If we had stopped because we had a line break then I cannot realistically
                // consume the end block element on this line. This would break editing when
                // the user hit shift-enter to put in a BR -- the P tag I was under has to
                // end in the next line since that is where the user wanted to see the caret.
                // IE bug 44561
                //
                if (pMe->_fLastWasBreak && _fIsEditable)
                    break;

                if (pFlowLayout->IsElementBlockInContext(pElement))
                {
                    const CFancyFormat * pFF = pNode->GetFancyFormat();
                    
                    pMe->_li._fHasEOP = TRUE;

                    CollectSpaceInfoFromEndNode(pNode, FALSE);

                    _fNoMarginAtBottom =    pElement->Tag() == ETAG_P
                                         && pElement->_fExplicitEndTag
                                         && !pFF->_fExplicitBottomMargin;

                    // does any blocks going out of scope force a page break
                    pMe->_li._fPageBreakAfter |= !!GET_PGBRK_AFTER(pFF->_bPageBreaks);

                }
                // Else do nothing, just continue looking ahead

                // Just verifies that an element is block within itself.
                Assert(ptp != ptpStop);
            }
            else if (ptp->IsBeginElementScope())
            {
                const CCharFormat *pCF = pNode->GetCharFormat();

                if (pCF->IsDisplayNone())
                {
                    LONG cchHidden = pMe->GetNestedElementCch(pElement, &ptp) - 1;

                    // Add the characters to the line.
                    pMe->_li._cch += cchHidden;

                    // Also add them to the whitespace of the line
                    pMe->_li._cchWhite += (SHORT)cchHidden;

                    // Add to cpCurrent
                    cpCurrent += cchHidden;
                }
                
                // We need to stop when we see a new block element
                else if (pFlowLayout->IsElementBlockInContext(pElement))
                {
                    pMe->_li._fHasEOP = TRUE;
                    break;
                }

                // Or a new layout (including aligned ones, since these
                // will now live on lines of their own.
                else if (pNode->NeedsLayout() || pNode->IsRelative())
                    break;
                else if (pElement->Tag() == ETAG_BR)
                    break;
                else if (!pNode->Element()->IsNoScope())
                    break;
            }
            
            if (_fIsEditable )
            {
                if (fConsumedFirstOne && ptp->ShowTreePos())
                    break;
                else
                    fConsumedFirstOne = TRUE;
            }
        }
        else
        {
            Assert(ptp->IsText());
            if (ptp->Cch())
                break;
        }

        {
            LONG cchAdvance = ptp->GetCch();
            
            // Add the characters to the line.
            pMe->_li._cch += cchAdvance;

            // Also add them to the whitespace of the line
            pMe->_li._cchWhite += (SHORT)cchAdvance;

            // Add to cpCurrent
            cpCurrent += cchAdvance;
        }
    }

    // The last paragraph of a layout shouldn't have this flag set
    if( ptp == ptpStop )
        pMe->_li._fHasEOP = FALSE;
        
    if (pMe->GetPtp() != ptp)
        pMe->SetPtp(ptp, cpCurrent);

    INSYNC(pMe);
}


//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::CalcBeforeSpace
//
//  Synopsis:   This function computes the before space of the line and remembers
//              the characters it has gone past in _li._cch.
//
//              Also computes border and padding (left and right too!) for
//              elements coming into scope.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CTreePos *
CRecalcLinePtr::CalcBeforeSpace(CLSMeasurer *pMe, BOOL fFirstLineInLayout)
{
    const CFancyFormat * pFF;
    const CParaFormat  * pPF;
    const CCharFormat  * pCF;
    CFlowLayout  *pFlowLayout = _pdp->GetFlowLayout();
    CDoc         *pDoc        = pFlowLayout->Doc();
    CTreeNode    *pNode       = NULL;
    CElement     *pElement;
    CTreePos     *ptpStop;
    CTreePos     *ptp;
    CTreePos     *ptpFormatting = NULL;
    BOOL          fSeenAbsolute = FALSE;
    BOOL          fSeenBeginBlockTag = FALSE;
                                  
    ptpStop = pMe->_pLS->_treeInfo._ptpLayoutLast;
#if DBG==1
    {
        CTreePos *ptpStartDbg;
        CTreePos *ptpStopDbg;
        pFlowLayout->GetContentTreeExtent(&ptpStartDbg, &ptpStopDbg);
        Assert(ptpStop == ptpStopDbg);
    }
#endif

    _xBordLeftPerLine = _xBordRightPerLine = _xPadLeftPerLine = _xPadRightPerLine = 0;
    
    if (fFirstLineInLayout)
    {
        LONG lPadding[PADDING_MAX];
        _pdp->GetPadding(_pci, lPadding, _pci->_smMode == SIZEMODE_MMWIDTH);
        if (pFlowLayout->ElementContent()->TestClassFlag(CElement::ELEMENTDESC_TABLECELL))
        {
            _lTopPadding = lPadding[PADDING_TOP];
        }
        else
        {
            _lPosSpace = max(_lPosSpace, lPadding[PADDING_TOP]);
            _lNegSpace = min(_lNegSpace, lPadding[PADDING_TOP]);
            _lTopPadding = 0;
        }
    }
    else
        _lTopPadding = 0;

    // initialize top padding & border
    _yPadTop = _yBordTop = 0;
    
    // BUGBUG (sujalp, ericvas, alexz) (fun)
    // this asserts too often - this needs to be fixed or assert removed
    // Assert(pMe->_li._cch == 0);
    for (ptp = pMe->GetPtp(); ; ptp = ptp->NextTreePos())
    {
        if (ptp->IsPointer())
            continue;

        if (ptp == ptpStop)
            break;
        
        if (ptp->IsNode())
        {
            if (_fIsEditable && ptp->ShowTreePos())
                pMe->_fMeasureFromTheStart = TRUE;

            pNode = ptp->Branch();
            pElement = pNode->Element();
            BOOL fNeedsLayout = pNode->NeedsLayout();
            if (ptp->IsEndElementScope())
            {
                if(pNode->IsRelative() && !fNeedsLayout)
                    pMe->_fRelativePreChars = TRUE;

                if (pFlowLayout->IsElementBlockInContext(pElement))
                {
                    //
                    // If we encounter a break on empty block end tag, then we should
                    // give vertical space otherwise a <X*></X> where X is a block element
                    // will not produce any vertical space. (Bug 45291).
                    //
                    if (  pElement->_fBreakOnEmpty
                        && (   fSeenBeginBlockTag
                            || (   pMe->_fLastWasBreak
                                && _fIsEditable
                               )
                           )
                       )
                    {
                        break;
                    }

                    if (fSeenAbsolute)
                        break;
                    
                    //
                    // If we are at an end LI it means that we have an
                    // empty LI for which we need to create an empty
                    // line. Hence we just break out of here. We will
                    // fall into the measurer with the ptp positioned
                    // at the end splay. The measurer will immly bail
                    // out, creating an empty line. CalcAfterSpace will
                    // go and then add the node char for the end LI to
                    // the line.
                    //
                    if (   pElement->HasFlag(TAGDESC_LISTITEM)
                        && (   pMe->_li._fHasBulletOrNum
                            || pMe->_fLastWasBreak
                           )
                       )
                    {
                        break;
                    }
                    
                    //
                    // Collect space info from the end node *only* if the previous
                    // line does not end in a BR. If it did, then the end block
                    // tag after does not contribute to the inter-paraspacing.
                    // (Remember, that a subsequent begin block tag will still
                    // contribute to the spacing!)
                    //
                    else if (!_fIsEditable || !pMe->_fLastWasBreak)
                    {
                        CollectSpaceInfoFromEndNode(pNode, fFirstLineInLayout, TRUE);
                    }

                }
                // Else do nothing, just continue looking ahead

                // Just verifies that an element is block within itself.
                Assert(ptp != ptpStop);
            }
            else if (ptp->IsBeginElementScope())
            {
                pCF = pNode->GetCharFormat();
                pFF = pNode->GetFancyFormat();
                pPF = pNode->GetParaFormat();

                if (pCF->IsDisplayNone())
                {
                    // The extra one is added in the normal processing.
                    pMe->_li._cch += pMe->GetNestedElementCch(pElement, &ptp) - 1;
                }
                else if (pElement->Tag() == ETAG_BR)
                {
                    break;
                }
                else
                {
                    if(pNode->IsRelative() && !fNeedsLayout)
                        pMe->_fRelativePreChars = TRUE;

                    if(pFF->_fClearLeft || pFF->_fClearRight)
                    {
                        _marginInfo._fClearLeft  |= pFF->_fClearLeft;
                        _marginInfo._fClearRight |= pFF->_fClearRight;
                    }

                    if (    pFlowLayout->IsElementBlockInContext(pElement)
                        &&  pElement->IsInlinedElement())
                    {
                        fSeenBeginBlockTag = TRUE;
                        
                        if (fSeenAbsolute)
                            break;
                        
                        LONG lFontHeight = pCF->GetHeightInTwips(pDoc);

                        if (pElement->HasFlag(TAGDESC_LIST))
                        {
                            Assert(pElement->IsBlockElement());
                            if (pMe->_li._fHasBulletOrNum)
                            {
                                ptpFormatting = ptp;
                                
                                do
                                {
                                    ptpFormatting = ptpFormatting->PreviousTreePos();
                                } while (ptpFormatting->GetCch() == 0);

                                break;
                            }
                        }

                        //
                        // NOTE(SujalP): Bug 38806 points out a problem where an
                        // abs pos'd LI does not have a bullet follow it. To fix that
                        // one we decided that we will _not_ draw the bullet for LI's
                        // with layout (&& !fHasLayout). However, 61373 and its dupes
                        // indicate that this is overly restrictive. So we will change
                        // this case and not draw a bullet for only abspos'd LI's.
                        //
                        else if (   pElement->Tag() == ETAG_LI
                                 && !pNode->IsAbsolute()
                                )
                        {
                            Assert(pElement->IsBlockElement());
                            pMe->_li._fHasBulletOrNum = TRUE;
                        }

                        // if a dd is comming into scope and is a
                        // naked DD, then compute the first line indent
                        if(     pElement->Tag() == ETAG_DD
                            &&  pPF->_fFirstLineIndentForDD)
                        {
                            CUnitValue cuv;
                        
                            cuv.SetPoints(LIST_INDENT_POINTS);
                            _xLeadAdjust += cuv.XGetPixelValue(_pci, 0, 1);
                        }

                        // if a block element is comming into scope, it better be
                        // the first line in the paragraph.
                        Assert(pMe->_li._fFirstInPara || pMe->_fLastWasBreak);
                        pMe->_li._fFirstInPara = TRUE;
                        
                        // compute padding and border height for the elements comming
                        // into scope
                        if(     !fNeedsLayout
                            &&   pPF->HasPadBord(FALSE))
                        {

                            if ( !pElement->_fDefinitelyNoBorders )
                            {
                                CBorderInfo borderinfo;

                                pElement->_fDefinitelyNoBorders = !GetBorderInfoHelper( pNode, _pci, &borderinfo, FALSE );
                                if ( !pElement->_fDefinitelyNoBorders )
                                {
                                    _yBordTop          += borderinfo.aiWidths[BORDER_TOP];
                                    _xBordLeftPerLine  += borderinfo.aiWidths[BORDER_LEFT];
                                    _xBordRightPerLine += borderinfo.aiWidths[BORDER_RIGHT];
                                }
                            }

                            _yPadTop += pFF->_cuvPaddingTop.YGetPixelValue(
                                                    _pci,
                                                    _pci->_sizeParent.cx, 
                                                    lFontHeight);
                            _xPadLeftPerLine  += pFF->_cuvPaddingLeft.XGetPixelValue(
                                                    _pci,
                                                    _pci->_sizeParent.cx, 
                                                    lFontHeight);
                            _xPadRightPerLine += pFF->_cuvPaddingRight.XGetPixelValue(
                                                    _pci,
                                                    _pci->_sizeParent.cx, 
                                                    lFontHeight);
                                                    
                            // If we have horizontal padding in percentages, flag the display
                            // so it can do a full recalc pass when necessary (e.g. parent width changes)
                            // Also see ApplyLineIndents() where we do this for horizontal indents.
                            if ( pFF->_cuvPaddingLeft.GetUnitType() == CUnitValue::UNIT_PERCENT ||
                                 pFF->_cuvPaddingRight.GetUnitType() == CUnitValue::UNIT_PERCENT )
                            {
                                _pdp->_fContainsHorzPercentAttr = TRUE;
                            }
                        }

                        // does any block elements comming into scope force a
                        // page break before this line
                        pMe->_li._fPageBreakBefore |= !!GET_PGBRK_BEFORE(pFF->_bPageBreaks);

                        // If we're the first line in the scope, we only care about our
                        // pre space if it has been explicitly set.
                        if ((   !pMe->_li._fHasBulletOrNum
                             && !fFirstLineInLayout
                            )
                            || pFF->_fExplicitTopMargin
                           )
                        {
                            // If this is in a PRE block, then we have no
                            // interline spacing.
                            if (!(   pNode->Parent()
                                  && pNode->Parent()->GetParaFormat()->HasPre(FALSE)
                                 )
                               )
                            {
                                LONG lTemp;

                                lTemp = pFF->_cuvSpaceBefore.YGetPixelValue(_pci,
                                                           _pci->_sizeParent.cx,
                                                           lFontHeight);

                                // Maintain the positives.
                                _lPosSpace = max(lTemp, _lPosSpace);

                                // Keep the negatives separately.
                                _lNegSpace =  min(lTemp, _lNegSpace);
                            }
                        }
                    }

                    //
                    // If we have hit a nested layout then we quit so that it can be measured.
                    // Note that we have noted the before space the site contributes in the
                    // code above.
                    //
                    // Absolute positioned nested layouts at BOL are a part of the pre-chars
                    // of the line. We also skip over them in FormattingNodeForLine.
                    //
                    if (fNeedsLayout)
                    {
                        //
                        // Should never be here for hidden layouts. They should be
                        // skipped over earlier in this function.
                        //
                        Assert(!pCF->IsDisplayNone());
                        if (pNode->IsAbsolute())
                        {
                            LONG cchElement = pMe->GetNestedElementCch(pElement, &ptp);

                            fSeenAbsolute = TRUE;
                            
                            // The extra one is added in the normal processing.
                            pMe->_li._cch +=  cchElement - 1;
                            pMe->_cchAbsPosdPreChars += cchElement; 
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            Assert(ptp->IsText());
            if (ptp->Cch())
                break;
        }

        pMe->_li._cch += ptp->GetCch();
    }

    if (ptp != pMe->GetPtp())
        pMe->SetPtp(ptp, -1);

    _xBordLeft  += _xBordLeftPerLine;
    _xBordRight += _xBordRightPerLine;
    _xPadLeft   += _xPadLeftPerLine;
    _xPadRight  += _xPadRightPerLine;
    
    return ptpFormatting ? ptpFormatting : ptp;
}

//+----------------------------------------------------------------------------
//
//  Member:     CRecalcLinePtr::CollectSpaceInfoFromEndNode
//
//  Synopsis:   Computes the space info when we are at the end of a block element.
//
//  Returns:    A BOOL indicating if any space info was collected.
//
//-----------------------------------------------------------------------------

BOOL
CRecalcLinePtr::CollectSpaceInfoFromEndNode(
    CTreeNode * pNode,
    BOOL        fFirstLineInLayout,
    BOOL        fPadBordForEmptyBlock)
{
    Assert(pNode);
    
    BOOL fRet = FALSE;
    const CFancyFormat *pFF = pNode->GetFancyFormat();
    CUnitValue cuv;
    
    Assert(   _pdp->GetFlowLayout()->IsElementBlockInContext(pNode->Element())
           || pNode->Element()->IsOwnLineElement(_pdp->GetFlowLayout())
          );
    CElement *pElement = pNode->Element();
    CDoc     *pDoc     = pElement->Doc();

    // Treading the fine line of Nav3, Nav4 and IE3 compat,
    // we include the bottom margin as long as we're not
    // the last line in the text site or not a P tag. This
    // is broadly Nav4 compatible.
    if (   !fFirstLineInLayout
        && (   pElement->_fExplicitEndTag
            || pFF->_fExplicitBottomMargin
           )
       )
    {
        // Deal with things proxied around text sites differently,
        // so we need to know when we're above the containing site.
        //if (pElement->GetLayout() == pFlowLayout)
        //break;

        // If this is in a PRE block, then we have no
        // interline spacing.
        if (!(   pNode->Parent()
              && pNode->Parent()->GetParaFormat()->HasPre(TRUE)
             )
           )
        {
            LONG lTemp;
            cuv = pFF->_cuvSpaceAfter;

            lTemp = cuv.YGetPixelValue(_pci,
                                       _pci->_sizeParent.cx,
                                       pNode->GetFontHeightInTwips(&cuv));

            _lPosSpace = max(lTemp, _lPosSpace);
            _lNegSpace = min(lTemp, _lNegSpace);
            if (pElement->Tag() != ETAG_P || pFF->_fExplicitBottomMargin)
            {
                _lPosSpaceNoP = max(lTemp, _lPosSpaceNoP);
                _lNegSpaceNoP = min(lTemp, _lNegSpaceNoP);
            }
        }
        fRet = TRUE;
    }

    // compute any padding or border height from elements
    // going out of scope
    if(     !pNode->NeedsLayout()
        &&   pNode->GetParaFormat()->HasPadBord(FALSE))
    {
        LONG lFontHeight = pNode->GetCharFormat()->GetHeightInTwips(pDoc);
        LONG xBordLeft, xBordRight, xPadLeft, xPadRight;

        xBordLeft = xBordRight = xPadLeft = xPadRight = 0;
        
        pFF = pNode->GetFancyFormat();

        if ( !pElement->_fDefinitelyNoBorders )
        {
            CBorderInfo borderinfo;

            pElement->_fDefinitelyNoBorders = !GetBorderInfoHelper( pNode, _pci, &borderinfo, FALSE );
            if ( !pElement->_fDefinitelyNoBorders )
            {
                if(fPadBordForEmptyBlock)
                {
                    _yBordTop   -= borderinfo.aiWidths[BORDER_TOP];
                }
                else
                {
                    _yBordBottom += borderinfo.aiWidths[BORDER_BOTTOM];
                }
                xBordLeft   = borderinfo.aiWidths[BORDER_LEFT];
                xBordRight  = borderinfo.aiWidths[BORDER_RIGHT];
            }
        }

        if(fPadBordForEmptyBlock)
        {
            // we run into this case if there are empty block elements
            // at the beginning of a line. So we need to subtract any
            // padding or border space accounted for this block element
            _yPadTop   -= pFF->_cuvPaddingTop.YGetPixelValue(
                                    _pci,
                                    _pci->_sizeParent.cx, 
                                    lFontHeight);
        }
        else
        {
            _yPadBottom += pFF->_cuvPaddingBottom.YGetPixelValue(
                                    _pci,
                                    _pci->_sizeParent.cx, 
                                    lFontHeight);
        }
        xPadLeft   = pFF->_cuvPaddingLeft.XGetPixelValue(
                                _pci,
                                _pci->_sizeParent.cx, 
                                lFontHeight);
        xPadRight  = pFF->_cuvPaddingRight.XGetPixelValue(
                                _pci,
                                _pci->_sizeParent.cx, 
                                lFontHeight);

        //
        // If fPadBordForEmptyBlock is true it means that we are called from CalcBeforeSpace.
        // During CalcBeforeSpace, the padding and border is collected in the per-line variables
        // and then accounted into _x[Pad|Bord][Left|Right] variables at the end of the call.
        // Hence when we are measuring for empty block elements, we remove their padding
        // and border from the perline varaibles, but when we are called from CalcAfterSpace
        // we remove it from the actual _x[Pad|Bord][Left|Right] variables.
        //
        if (fPadBordForEmptyBlock)
        {
            _xBordLeftPerLine  -= xBordLeft;
            _xBordRightPerLine -= xBordRight;
            _xPadLeftPerLine   -= xPadLeft;
            _xPadRightPerLine  -= xPadRight;
        }
        else
        {
            _xBordLeft  -= xBordLeft;
            _xBordRight -= xBordRight;
            _xPadLeft   -= xPadLeft;
            _xPadRight  -= xPadRight;
        }
    }

    return fRet;
}
