
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltcell.cxx
//
//  Contents:   Implementation of CTableCellLayout and related classes.
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
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

ExternTag(tagTableRecalc);
ExternTag(tagTableCalc);

extern WORD s_awEdgesFromTableFrame[htmlFrameborder+1];
extern WORD s_awEdgesFromTableRules[htmlRulesall+1];

MtDefine(CTableCellLayout, Layout, "CTableCellLayout")

const CLayout::LAYOUTDESC CTableCellLayout::s_layoutdesc =
{
    LAYOUTDESC_TABLECELL    |
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};


//+------------------------------------------------------------------------
//
//  Member:     CTableCellLayout::GetAutoSize, CTxtEdit
//
//  Synopsis:   Return if autosize
//
//  Returns:    TRUE if autosize
//
//-------------------------------------------------------------------------

BOOL
CTableCellLayout::GetAutoSize() const
{
    return _fContentsAffectSize;
}


//+----------------------------------------------------------------------------
//
//  Member:     DoLayout
//
//  Synopsis:   Layout contents
//
//  Arguments:  grfLayout   - One or more LAYOUT_xxxx flags
//
//-----------------------------------------------------------------------------
void
CTableCellLayout::DoLayout(
    DWORD   grfLayout)
{
    _fDelayCalc = FALSE;
    super::DoLayout(grfLayout);
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
CTableCellLayout::Notify(
    CNotification * pnf)
{
    Assert(!pnf->IsReceived(_snLast));

    // ignore focus notifications
    Assert(pnf->Element() != ElementOwner()         ||
           pnf->IsType(NTYPE_ELEMENT_REMEASURE)     ||
           pnf->IsType(NTYPE_DISPLAY_CHANGE)        ||
           pnf->IsType(NTYPE_VISIBILITY_CHANGE)     ||
           pnf->IsType(NTYPE_ELEMENT_MINMAX)        ||
           pnf->IsType(NTYPE_ELEMENT_ENSURERECALC)  ||
           pnf->IsType(NTYPE_ELEMENT_ADD_ADORNER)   ||
           IsInvalidationNotification(pnf));

    BOOL            fWasDirty = IsDirty() || _fSizeThis;
    CTable *        pTable    = Table();
    CTableLayout *  pTableLayout   = pTable
                                    ? pTable->Layout()
                                    : NULL;
    BOOL            fNeedResize = FALSE;


    //
    //  Stand-alone cells behave like normal text containers
    //

    if (!pTable)
    {
        super::Notify(pnf);
        return;
    }

    if (pnf->IsType(NTYPE_ELEMENT_MINMAX))
    {
        if (pTableLayout)
            pTableLayout->_fDontSaveHistory = TRUE;
    }
    //
    //  First, start with default handling
    //  (But prevent posting a layout request)
    //

    if (    (   pTableLayout
            &&  pTableLayout->CanRecalc())
        ||  (   !pnf->IsType(NTYPE_ELEMENT_ENSURERECALC)
            &&  !pnf->IsType(NTYPE_RANGE_ENSURERECALC)))
    {
        CSaveNotifyFlags    snf(pnf);

        pnf->SetFlag(NFLAGS_DONOTLAYOUT);
        super::Notify(pnf);
        if (   Doc()->_fDesignMode 
            && (     pnf->IsTextChange()
                 ||  pnf->IsType(NTYPE_ELEMENT_REMEASURE)
                 ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)
                 ||  pnf->IsType(NTYPE_CHARS_RESIZE)))
        {
            pTableLayout->ResetMinMax();
        }
    }

    if ( pnf->IsType(NTYPE_DISPLAY_CHANGE) )
    {
        if ((pnf->Element() == ElementOwner())
            || (pnf->Element())->IsRoot())             // this case covers disply change on the style sheet. (bug #77806)
                                            // BUGBUG: what we should really check here is if the style sheet display changes
                                            // and this is a cell's style, then call HandlePositionDisplayChnage()
        {
            HandlePositionDisplayChange();
        }
        else
        {
            fNeedResize = TRUE;
        }
    }
    //
    //  Resize the cell if content has changed
    //  BUGBUG: We could do better than this for "fixed" tables (brendand)
    //

    if (    !fWasDirty
        &&  (   IsDirty()
            ||  fNeedResize
            ||  (   pnf->IsType(NTYPE_ELEMENT_MINMAX)   // inside of me there is an element who needs to be min-maxed
                &&  !TestLock(CElement::ELEMENTLOCK_SIZING)
                &&  !pnf->Element()->IsAbsolute())))
    {
        Assert(!_fSizeThis);
        Assert(!TestLock(CElement::ELEMENTLOCK_SIZING));

        //
        //  Resize the cell
        //  (If the notification is not being forwarded, then take it over;
        //   otherwise, post a new, resize notification)
        //

        if (    pnf->IsFlagSet(NFLAGS_SENDUNTILHANDLED)
            &&  pnf->IsHandled())
        {
            pnf->ChangeTo(NTYPE_ELEMENT_RESIZE, ElementOwner());
        }
        else
        {
            ElementOwner()->ResizeElement();
        }
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CTableCellLayout::HandlePositionDisplayChange
//
//  Synopsis:   Process position/display property changes on the cell
//
//-------------------------------------------------------------------------

void
CTableCellLayout::HandlePositionDisplayChange()
{
    CTable *pTable = Table();
    CTableLayout *pTableLayout = pTable? pTable->Layout() : NULL;

    if (pTableLayout)
    {
        if (Tag() == ETAG_TD || Tag() == ETAG_TH)
        {
            CTableRow *pRow = Row();
            if (pRow)
            {
                // neigboring cells in the row need to update their caches
                CTableRowLayout *pRowLayout = pRow->Layout();
                if (pRowLayout)
                {
                    for (int i= _iCol; i < pRowLayout->_aryCells.Size(); i++)
                    {
                        CTableCell *pCell = pRowLayout->_aryCells[i];
                        if (IsReal(pCell))
                        {
                            pCell->EnsureFormatCacheChange (ELEMCHNG_CLEARCACHES);
                        }
                    }
                }
            }
        }
        pTableLayout->MarkTableLayoutCacheDirty();
        _fSizeThis = TRUE; // all we want is to enusre disp node (ensure correct layer ordering).
        pTableLayout->Resize();
    }
    
    return;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableCellLayout::CalcSize, CTableCellLayout
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

DWORD
CTableCellLayout::CalcSize( CCalcInfo * pci,
                            SIZE *      psize,
                            SIZE *      psizeDefault)
{
    CSaveCalcInfo   sci(pci, this);
    CScopeFlag      csfCalcing(this);
    DWORD           grfReturn;
    int             cx = psize->cx;
    CTable        * pTable;
    CTableLayout  * pTableLayout;
    CDisplay      * pdp = GetDisplay();
    CTreeNode     * pNodeSelf = GetFirstBranch();
    SIZE            sizeDefault;
    ELEMENT_TAG     etag = Tag();
    BOOL            fIgnoreZeroWidth = FALSE;
    BOOL            fFixedSizeCell = FALSE;
    CTableCell    * pCell;
    BOOL            fSetSize = FALSE;
    BOOL            fSetCellPosition = FALSE;

    if (pci->_fTableCalcInfo)
    {
        CTableCalcInfo * ptci = (CTableCalcInfo *) pci;
        
        pTable       = ptci->Table();
        pTableLayout = ptci->TableLayout();
        fSetCellPosition = ptci->_fSetCellPosition;
    }
    else
    {
        pTable = Table();
        if (pTable)
            pTableLayout = pTable->Layout();
        else
        {
            // this cell is not in the table (therefore it is a most likely DOM created homeless cell).
            *psize    =
            _sizeCell = g_Zero.size;
            return pci->_grfLayout;
        }
    }

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    if(_pDispNode && (_pDispNode->IsRightToLeft() != (pNodeSelf->GetCascadedBlockDirection() == styleDirRightToLeft)))
        pci->_grfLayout |= LAYOUT_FORCE;

    TraceTag((tagTableCalc, "CTableCellLayout::CalcSize - Enter (0x%x), Table = 0x%x, smMode = %x, grfLayout = %x", this, pTable, pci->_smMode, pci->_grfLayout));

    // BUGBUG - HACK - (srinib) - ComputeFormats would be a good place for this
    // but right now it is crashing when trying to access the display
    // After joe fixes his tree code. (insert the node into the tree as
    // soon as the element is parsed).
    Assert (pdp);
    pdp->SetWordWrap(!pNodeSelf->GetCharFormat()->HasNoBreak(TRUE));

    // BUGBUG: Alex disagrees with this. (OliverSe)
    // pdp->SetCaretWidth(0);  // NOTE: for cells we don't adjust the width of the cells for the caret (_dxCaret = 0).

    pCell = TableCell();

    Assert(pci);
    Assert(psize);
    Assert(pci->_smMode == SIZEMODE_MMWIDTH || 
           pTable->TestLock(CElement::ELEMENTLOCK_SIZING) || 
           (pTableLayout && pTableLayout->IsFixed() && !_fDelayCalc) ||
           pCell->IsAbsolute());
    Assert(pTableLayout && pTableLayout->IsCalced());

    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);

    if (pci->_grfLayout & LAYOUT_FORCE)
    {
        _fSizeThis         = TRUE;
        _fAutoBelow        = FALSE;
        _fPositionSet      = FALSE;
        _fContainsRelative = FALSE;
    }

    Assert (pTableLayout && pTableLayout->_cNestedLevel != -1);
    if (!pTableLayout || pNodeSelf->IsDisplayNone() || pTableLayout->_cNestedLevel > SECURE_NESTED_LEVEL)
    {
        *psize    =
        _sizeCell = g_Zero.size;
        return grfReturn;
    }


    if (!_fDelayCalc)
    {
#if DBG == 1
        if (pci->_smMode == SIZEMODE_NATURAL)
        {
            pci->_yBaseLine = -1;
        }
#endif      
        grfReturn = super::CalcSize(pci, psize, &sizeDefault);
    }
    else
    {
        _fSizeThis = FALSE;
    }

    if (pTableLayout->IsFixed() && !IsCaption(etag))
    {
        // For the fixed style tables, regardless to the MIN width we might clip the content of the cell
        CTableRowLayout *   pRowLayout = Row()->Layout();
        if (psize->cx > cx)
        {
            // Check if we need to clip the content of the cell
            const CFancyFormat *  pFF = pNodeSelf->GetFancyFormat();
            if (pFF->_bOverflowX != styleOverflowVisible)
            {
                // we need to clip the content of the cell
                EnsureDispNodeIsContainer();
                fSetSize = TRUE;
            }
        }
        _sizeCell.cx =
        psize->cx    = cx;
        if (pRowLayout->IsHeightSpecifiedInPixel())
        {
            if (pCell->RowSpan() == 1)
            {
                _sizeCell.cy     = min(psize->cy, pRowLayout->_yHeight);
                psize->cy        = pRowLayout->_yHeight;
            }
            else
            {
                if (!fSetCellPosition)  // don't reset _sizeCell during set cell position (bug #47838)
                {
                    _sizeCell.cy = psize->cy;
                }
            }
            fFixedSizeCell = TRUE;
            CDispNode * pDispNode = GetElementDispNode();
            pDispNode->SetSize(*psize, FALSE);
            fIgnoreZeroWidth = TRUE;
        }
    }

    if (pci->_smMode == SIZEMODE_NATURAL ||
        pci->_smMode == SIZEMODE_SET      )
    {
        CDispNode * pDispNode = GetElementDispNode();

        if (grfReturn & LAYOUT_THIS)
        {
            CBorderInfo bi(FALSE); // no init

            if (pci->_smMode != SIZEMODE_SET)
            {

                // If the cell is empty, set its height to zero.
                if ((NoContent() && !fIgnoreZeroWidth) && (!pTable->IsDatabound() || pTableLayout->IsRepeating()))
                {
                    // Null out cy size component.
                    psize->cy = 0;
                    fSetSize  = TRUE;
                }

                // If the cell has no height, but does have borders
                // Then ensure enough space for the horizontal borders
                if (    psize->cy == 0
                    &&  pci->_smMode == SIZEMODE_NATURAL
                    &&  GetCellBorderInfo(pci, &bi, FALSE, 0, pTable, pTableLayout))
                {
                    psize->cy += bi.aiWidths[BORDER_TOP] + bi.aiWidths[BORDER_BOTTOM];
                    fSetSize   = TRUE;
                }
                
                // At this point, we should have a display node.
                if (    fSetSize
                    &&  pDispNode)
                {
                    pDispNode->SetSize(*psize, FALSE);
                }

                Assert(pTable->TestLock(CElement::ELEMENTLOCK_SIZING) || pTableLayout->IsFixedBehaviour() || pCell->IsAbsolute());
            }

            Assert(pci->_yBaseLine != -1);
            _yBaseLine = pci->_yBaseLine;

            //
            // Save the true height of the cell (which may differ from the height of the containing
            // row and thus the value kept in _sizeProposed); However, only cache this value when
            // responding to a NATURAL size request or if the cell contains children whose
            // heights are a percentage of the cell (since even during a SET operation those can
            // change in size thus affecting the cell size)
            //

            if (    !fFixedSizeCell
                &&  (
                    pci->_smMode == SIZEMODE_NATURAL
                ||  _fChildHeightPercent
                ||  ContainsVertPercentAttr()))
            {
                _sizeCell = *psize;
            }
            Assert(pTable->TestLock(CElement::ELEMENTLOCK_SIZING) || (pTableLayout->IsFixed() && !_fDelayCalc) || pCell->IsAbsolute());

            const CFancyFormat *  pFF = pNodeSelf->GetFancyFormat();

            if (   (    (_fContainsRelative || _fAutoBelow)
                     && !ElementOwner()->IsZParent() )  
                || pFF->_fPositioned)
            {
                CLayout *pLayout = GetUpdatedParentLayout();
                pLayout->_fAutoBelow = TRUE;
            }
        }

        //
        // Set the inset offset for the display node content
        //

        if (    (   pci->_smMode == SIZEMODE_NATURAL
                ||  pci->_smMode == SIZEMODE_SET
                ||  pci->_smMode == SIZEMODE_FULLSIZE)
            &&  pDispNode
            &&  pDispNode->HasInset())
        {
            styleVerticalAlign  va;
            htmlCellVAlign      cellVAlign;
            long                cy;

            cellVAlign = (htmlCellVAlign)pNodeSelf->GetParaFormat()->_bTableVAlignment;

            if (    cellVAlign != htmlCellVAlignBaseline
                ||  Tag()      == ETAG_CAPTION)
            {
                va = (cellVAlign == htmlCellVAlignMiddle
                        ? styleVerticalAlignMiddle
                        : cellVAlign == htmlCellVAlignBottom
                                ? styleVerticalAlignBottom
                                : styleVerticalAlignTop);
                cy = _sizeCell.cy;
            }
            else
            {
                int             yBaselineRow = pCell->Row()->Layout()->_yBaseLine;

                if (    _yBaseLine   != -1
                    &&  yBaselineRow != -1)
                {
                    
                    va = styleVerticalAlignBaseline;

                    cy = yBaselineRow - _yBaseLine;
                    if (cy + _sizeCell.cy > psize->cy)
                    {
                        cy = psize->cy - _sizeCell.cy;
                    }
                }
                else
                {
                    va = styleVerticalAlignTop;
                    cy = _sizeCell.cy;
                }
            }

            SizeDispNodeInsets(va, cy);
        }


#ifdef NO_DATABINDING
        // Assert(_fMinMaxValid || pTableLayout->IsFixedBehaviour());
#else
        // BUGBUG: 70458, need to investigate further why the MinMax is invalid.
        // Assert(_fMinMaxValid || IsGenerated() || pTableLayout->IsFixedBehaviour() || pCell->IsAbsolute());
#endif
        // BUGBUG: alexa: the following assert breaks Final96 page (manual DRT), need to investigate.
        // Assert(_xMin <= _sizeCell.cx);

        pci->_yBaseLine = _yBaseLine;
    }

    else if (  pci->_smMode == SIZEMODE_MMWIDTH
            || pci->_smMode == SIZEMODE_MINWIDTH
        )
    {
        const CFancyFormat *  pFF = pNodeSelf->GetFancyFormat();
        
        // Need to set this here because expressions aren't calculated
        // until long after parsing (which is when we really want to
        // calculate this).
        if (!IsCaption(pCell->Tag()) && !pCell->Row()->_fAdjustHeight)
        {
            const   CHeightUnitValue * puvHeight;
            puvHeight = (CHeightUnitValue *)&pFF->_cuvHeight;
            if (puvHeight->IsSpecified())
                pCell->Row()->_fAdjustHeight = TRUE;
        }

        //
        // NETSCAPE: If NOWRAP was specified along with a fixed WIDTH,
        //           use the fixed WIDTH as a min/max (if not smaller than the content)
        //

        if (pNodeSelf->GetFancyFormat()->_fHasNoWrap)
        {
            if (!pFF->_cuvWidth.IsNullOrEnum() &&
                 pFF->_cuvWidth.GetUnitType() != CUnitValue::UNIT_PERCENT)
            {
                psize->cx =
                psize->cy =
                _xMax     =
                _xMin     = max((long)_xMin,
                                 pFF->_cuvWidth.XGetPixelValue(pci, 0,
                                          pNodeSelf->GetFontHeightInTwips((CUnitValue *)&pFF->_cuvWidth)));
            }
        }

    }

    Assert(pci->_smMode != SIZEMODE_NATURAL || pci->_yBaseLine >= 0 || pTableLayout->IsFixedBehaviour());

    TraceTag((tagTableCalc, "CTableCell::CalcSize - Exit (0x%x)", this));
    return grfReturn;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableCellLayout::GetBorderAndPaddingWidth
//
//  Synopsis:   Calculate the size of the horizontal left border, right
//              right border, and paddings for this cell.
//
//--------------------------------------------------------------------------

int CTableCellLayout::GetBorderAndPaddingWidth( CDocInfo *pdci, BOOL fOnlyBorderWidths )
{
    CBorderInfo     borderinfo(FALSE);
    CTable *        pTable = Table();
    CTableLayout *  pTableLayout = pTable->Layout();
    int             acc = 0;

    if (!fOnlyBorderWidths)
    {
        CTreeNode *             pTreeNode = ElementOwner()->GetFirstBranch();
        const CFancyFormat *    pFF       = pTreeNode->GetFancyFormat();

        acc = pFF->_cuvPaddingLeft.XGetPixelValue(pdci, 0, pTreeNode->GetFontHeightInTwips((CUnitValue*)this))
            + pFF->_cuvPaddingRight.XGetPixelValue(pdci, 0, pTreeNode->GetFontHeightInTwips((CUnitValue*)this));
    }

    if (GetCellBorderInfo(pdci, &borderinfo, FALSE, 0, pTable, pTableLayout))
    {
        BOOL fPrinting = pdci && pdci->_pDoc && pdci->_pDoc->IsPrintDoc();
        int  accBorders = borderinfo.aiWidths[BORDER_RIGHT] + borderinfo.aiWidths[BORDER_LEFT];

        if (fPrinting)
            accBorders = pdci->DocPixelsFromWindowX(accBorders);

        acc += accBorders;
    }

    return acc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableCellLayout::CalcSizeAtUserWidth
//
//  Synopsis:   Calculate the size of the object based applying the user's
//              specified width. This function is invented to match Netscape
//              behaviour, who are respecting the user width for laying out
//              the text (line braking is done on the user's specified width)
//              regardless to the calculated size of the cell.
//
//--------------------------------------------------------------------------

DWORD
CTableCellLayout::CalcSizeAtUserWidth(CCalcInfo * pci, SIZE * psize)
{
    int     cx          = psize->cx;
    int     cxUserWidth = GetSpecifiedPixelWidth(pci);
    BOOL    fAdjustView = FALSE;
    CTableLayout * pTableLayout;
    DWORD   grfReturn;
#if DBG == 1
    int     cxMin;
#endif

    if (pci->_fTableCalcInfo)
    {
        CTableCalcInfo * ptci = (CTableCalcInfo *) pci;
        
        pTableLayout = ptci->TableLayout();
    }
    else
    {
        pTableLayout = Table()->Layout();
    }

    //
    // If a non-inherited user set value exists, respect the User's size and calculate
    // the cell with that size, but set the different view for the cell.
    // NOTE: This is only applys to cells in columns without a fixed
    //       size (that is, covered by the COLS attribute)
    //

    if (pTableLayout
        &&  _iCol >= pTableLayout->_cSizedCols
        &&  cxUserWidth
        &&  !_fInheritedWidth
        &&  cxUserWidth < cx)
    {
        // We should not try to render the cell in a view less then insets of the cell + 1 pixel

#if DBG==1
        cxMin = GetBorderAndPaddingWidth( pci ) + 1;
        Assert (cxMin <= cxUserWidth);
#endif
        psize->cx   = cxUserWidth;
        fAdjustView = TRUE;
    }

    grfReturn = CalcSize(pci, psize);

    //
    // Re-adjust the view width if necessary
    //

    if (fAdjustView)
    {
        if (pTableLayout->IsFixed() && !IsCaption(Tag()))
        {
            // For the fixed style tables, regardless to the MIN width we might clip the content of the cell
            // Check if we need to clip the content of the cell
            CTreeNode   * pNodeSelf = GetFirstBranch();
            const CFancyFormat *  pFF = pNodeSelf->GetFancyFormat();
            if (pFF->_bOverflowX == styleOverflowVisible)
            {
                CSize sizeCell;
                GetSize(&sizeCell);
                if (sizeCell.cx >= cx)
                {
                    fAdjustView = FALSE;
                }
            }
        }
        psize->cx = cx;
        if (fAdjustView)
        {
            SizeDispNode(pci, *psize);
            SizeContentDispNode(CSize(_dp.GetMaxWidth(), _dp.GetHeight()));
        }
    }

    return grfReturn;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableCellLayout::Resize
//
//  Synopsis:   request to resize cell layout
//
//----------------------------------------------------------------------------

void
CTableCellLayout::Resize()
{
#if DBG == 1
    if (!IsTagEnabled(tagTableRecalc))
#endif
    if (TableLayout() && 
        !TableLayout()->_fCompleted)
        return;

    ElementOwner()->ResizeElement();
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClientBorder
//
//  Synopsis:   Draw the border
//
//  Arguments:  prcBounds       bounding rect of display leaf
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CTableCellLayout::DrawClientBorder(const RECT * prcBounds, const RECT * prcRedraw, CDispSurface * pDispSurface, CDispNode * pDispNode, void * pClientData, DWORD dwFlags)
{
    CElement * pElement;
    CTreeNode * pNode;
    const CFancyFormat * pFF;

    if (NoContent())
        return;

    pElement = ElementOwner();

    if (pElement->Tag() != ETAG_CAPTION)
    {
        pNode = pElement->GetFirstBranch();
        if (!pNode)
            return;

        pFF = pNode->GetFancyFormat();

        if (pFF->_bDisplay == styleDisplayNone)
            return;

        if (   pFF->_bPositionType != stylePositionabsolute
            && pFF->_bPositionType != stylePositionrelative)
        {
            CTableLayout * pTableLayout;

            pTableLayout = Table()->Layout();
            if (pTableLayout->_fCollapse || pTableLayout->_fRuleOrFrameAffectBorders)
                return;
        }
    }

    super::DrawClientBorder(prcBounds, prcRedraw, pDispSurface, pDispNode, pClientData, dwFlags);
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableCellLayout::DrawBorderHelper
//
//  Synopsis:   Paint the table cell's border if it has one.  Called from
//              CTableCellLayout::Draw.
//
//----------------------------------------------------------------------------

#define BorderFlag(border) \
   ((border == BORDER_LEFT) ? BF_LEFT : \
   ((border == BORDER_TOP) ? BF_TOP : \
   ((border == BORDER_RIGHT) ? BF_RIGHT : BF_BOTTOM)))
#define BorderOrientation(border) ((border == BORDER_LEFT || border == BORDER_TOP)?1:-1)
#define TopLeft(border) (border == BORDER_LEFT || border == BORDER_TOP)
#define TopRight(border) (border == BORDER_RIGHT || border == BORDER_TOP)

void
CTableCellLayout::DrawBorderHelper(CFormDrawInfo *pDI, BOOL * pfShrunkDCClipRegion)
{
    CTable *        pTable = Table();
    CTableLayout *  pTableLayout = pTable->Layout();
    htmlRules       trRules = pTable->GetAArules();
    CBorderInfo     borderinfo(FALSE);  // no init
    CRect           rcInset;
    CRect           rcBorder;
    RECT            rcBorderVisible;
    WORD            grfBorderCollapseAdjustment = 0;
    ELEMENT_TAG     etag = Tag();
    CDispNode *     pDispNode = GetElementDispNode();
    BOOL            fRTLTable = pTableLayout->GetElementDispNode()->IsRightToLeft();
    // The following two variables are used for correcting border connections in right to left tables
    // This is a cascading hack that precipitates from the way borders are drawn. If the cell connects
    // to the table's border on either the left or right side, the flag will be set to alert us to
    // do the adjustment just before we draw.
    BOOL            fRTLLeftAdjust = FALSE, fRTLRightAdjust = FALSE;

    Assert(pTableLayout->_fCollapse || pTableLayout->_fRuleOrFrameAffectBorders);
    Assert(pfShrunkDCClipRegion);

    if (!pDispNode)
    {
        return;
    }

    pDispNode->GetBounds(&rcInset);

    rcBorder = rcInset;

    GetCellBorderInfo(pDI, &borderinfo, TRUE, pDI->GetDC(), pTable, pTableLayout, pfShrunkDCClipRegion);
    if (borderinfo.wEdges)
    {
        // Scale borders to device.
        borderinfo.aiWidths[BORDER_TOP]    = pDI->DocPixelsFromWindowY( borderinfo.aiWidths[BORDER_TOP]    );
        borderinfo.aiWidths[BORDER_RIGHT]  = pDI->DocPixelsFromWindowX( borderinfo.aiWidths[BORDER_RIGHT]  );
        borderinfo.aiWidths[BORDER_BOTTOM] = pDI->DocPixelsFromWindowY( borderinfo.aiWidths[BORDER_BOTTOM] );
        borderinfo.aiWidths[BORDER_LEFT]   = pDI->DocPixelsFromWindowX( borderinfo.aiWidths[BORDER_LEFT]   );
    }

    if (pTableLayout->_fCollapse && !IsCaption(etag))
    {
        int border;
        int widthBorderHalfInset;
        LONG *pnInsetSide, *pnBorderSide;

        for (border = BORDER_TOP ; border <= BORDER_LEFT ; border++)
        {
            BOOL fCellBorderAtTableBorder = FALSE;

            // Set up current border for generic processing.
            switch (border)
            {
            case BORDER_LEFT:
                pnInsetSide = &(rcInset.left);
                pnBorderSide = &(rcBorder.left);
                if(!fRTLTable)
                    fCellBorderAtTableBorder = (_iCol == 0);
                else
                    fRTLLeftAdjust = fCellBorderAtTableBorder = (_iCol+TableCell()->ColSpan() == pTableLayout->_cCols);
                break;
            case BORDER_TOP:
                pnInsetSide = &(rcInset.top);
                pnBorderSide = &(rcBorder.top);
                fCellBorderAtTableBorder = (Row()->Layout()->_iRow == 0);
                break;
            case BORDER_RIGHT:
                pnInsetSide = &(rcInset.right);
                pnBorderSide = &(rcBorder.right);
                if(!fRTLTable)
                    fCellBorderAtTableBorder = (_iCol+TableCell()->ColSpan() == pTableLayout->_cCols);
                else
                    fRTLRightAdjust = fCellBorderAtTableBorder = (_iCol == 0);
                break;
            default: // case BORDER_BOTTOM:
                pnInsetSide = &(rcInset.bottom);
                pnBorderSide = &(rcBorder.bottom);
                fCellBorderAtTableBorder = (Row()->Layout()->_iRow+TableCell()->RowSpan()-1 == pTableLayout->GetLastRow());
                break;
            }

            // If the cell border coincides with the table border, no
            // collapsing takes place because the table border takes
            // precedence over cell borders.
            if (fCellBorderAtTableBorder)
            {
                if (!borderinfo.aiWidths[border])
                {
                    grfBorderCollapseAdjustment |= BorderFlag(border);
                }
                *pnInsetSide += borderinfo.aiWidths[border] * BorderOrientation(border); // * +/-1
                continue;
            }

            // Half the border width (signed) is what we are going to modify the rects by.
            widthBorderHalfInset = ((borderinfo.aiWidths[border]+(!fRTLTable ? (TopLeft(border)?1:0) : (TopRight(border)?1:0)))>>1) * BorderOrientation(border); // * +/-1

            // The inset rect shrinks only by half the width because our neighbor
            // accomodates the other half.
            *pnInsetSide += widthBorderHalfInset;

            // If we are rendering our own border along this border...
            if (borderinfo.wEdges & BorderFlag(border))
            {
                // The rcBorder grows by half the width so we can draw outside our rect.
                *pnBorderSide -= ((borderinfo.aiWidths[border]+(!fRTLTable ? (TopLeft(border)?0:1) : (TopRight(border)?0:1)))>>1) * BorderOrientation(border); // * +/-1
            }
            else
            {
                // The rcBorder shrinks (just like the inset rect) by half to let the
                // neighbor draw half of its border in this space.
                *pnBorderSide += widthBorderHalfInset;

                // Make sure border rendering code doesn't get confused by neighbor's
                // measurements.
                borderinfo.aiWidths[border] = 0;
                grfBorderCollapseAdjustment |= BorderFlag(border);
            }
        }
    }
    else
    {
        if (pTableLayout->_fCollapse)
        {
            Assert(IsCaption(etag));
            int border = (DYNCAST(CTableCaption, TableCell())->IsCaptionOnBottom()) ? BORDER_TOP : BORDER_BOTTOM;

            if (!borderinfo.aiWidths[border])
                grfBorderCollapseAdjustment |= BorderFlag(border);
        }

        rcInset.left   += borderinfo.aiWidths[BORDER_LEFT];
        rcInset.top    += borderinfo.aiWidths[BORDER_TOP];
        rcInset.right  -= borderinfo.aiWidths[BORDER_RIGHT];
        rcInset.bottom -= borderinfo.aiWidths[BORDER_BOTTOM];
    }

    //
    // Determine the RECT for and, if necessary, draw borders
    //

    if (trRules != htmlRulesNotSet)
    {
        if (!(borderinfo.wEdges & BF_TOP))
            rcBorder.top -= pTableLayout->CellSpacingY();
        if (!(borderinfo.wEdges & BF_BOTTOM))
            rcBorder.bottom += pTableLayout->CellSpacingY();
        if (!(borderinfo.wEdges & BF_LEFT))
            rcBorder.left -= pTableLayout->CellSpacingX();
        if (!(borderinfo.wEdges & BF_RIGHT))
            rcBorder.right += pTableLayout->CellSpacingX();
    }

    //
    // Render the border if visible.
    //

    rcBorderVisible = rcBorder;

    if (rcBorderVisible.left < rcBorderVisible.right  &&
        rcBorderVisible.top  < rcBorderVisible.bottom &&
        (rcBorderVisible.left   < rcInset.left  ||
         rcBorderVisible.top    < rcInset.top   ||
         rcBorderVisible.right  > rcInset.right ||
         rcBorderVisible.bottom > rcInset.bottom ))
    {
        // Adjust for collapsed pixels on all edges drawn by
        // neighbors (or the table border).  We have to do this,
        // so that the border drawing code properly attaches our
        // own borders with the neighbors' borders.  This has to
        // happen after the clipping above.
        if (grfBorderCollapseAdjustment)
        {
            if (grfBorderCollapseAdjustment & BF_LEFT)
                rcBorder.left -= (fRTLLeftAdjust?0:1);
            if (grfBorderCollapseAdjustment & BF_TOP)
                rcBorder.top--;
            if (grfBorderCollapseAdjustment & BF_RIGHT)
                rcBorder.right += (fRTLRightAdjust?2:1);
            if (grfBorderCollapseAdjustment & BF_BOTTOM)
                rcBorder.bottom++;
        }

        ::DrawBorder(pDI, &rcBorder, &borderinfo);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     Draw
//
//  Synopsis:   Paint the object.
//
//----------------------------------------------------------------------------

void
CTableCellLayout::Draw(CFormDrawInfo *pDI, CDispNode *pDispNode)
{
    if (_fDelayCalc)
    {
        PostLayoutRequest(LAYOUT_FORCEFIRSTCALC | LAYOUT_MEASURE);    //  | LAYOUT_BACKGROUND);
    }
    super::Draw(pDI, pDispNode);
}


//+------------------------------------------------------------------------
//
//  Member:     CTableCellLayout::PaintSelectionFeedback
//
//  Synopsis:   Paints the object's selection feedback, if it exists and
//              painting it is appropriate
//
//  Arguments:  hdc         HDC to draw on.
//              prc         Rect to draw in
//              dwSelInfo   Additional info about the selection
//
//-------------------------------------------------------------------------

void
CTableCellLayout::PaintSelectionFeedback(CFormDrawInfo *pDI, RECT *prc, DWORD dwSelInfo)
{
    // no selection feedback on table cells
}


//+------------------------------------------------------------------------
//
//  Member:     CTableCellLayout::GetSpecifiedPixelWidth
//
//  Synopsis:   get user specified pixel width
//
//  Returns:    returns user's width of the cell (0 if not set or
//              specified in %%)
//              if user set's width <= 0 it will be ignored
//-------------------------------------------------------------------------

int
CTableCellLayout::GetSpecifiedPixelWidth(CDocInfo * pdci)
{
    int iUserWidth = 0;

    // If a user set value exists, respect the User's size and calculate
    // the cell with that size, but set the different view for the cell.

    CTableCell *pCell = TableCell();

    const CWidthUnitValue *punit = (CWidthUnitValue *)&pCell->GetFirstBranch()->GetFancyFormat()->_cuvWidth;

    if (punit->IsSpecified() && punit->IsSpecifiedInPixel())
    {
        iUserWidth = punit->GetPixelWidth(pdci, pCell);
        if (iUserWidth <= 0)    // ignore 0-width
        {
            iUserWidth = 0;
        }
        else
        {
            iUserWidth += GetBorderAndPaddingWidth(pdci);
        }
    }

    return iUserWidth;
}


//+------------------------------------------------------------------------
//
//  Member:     CTableCellLayout::GetMaxLineWidth
//
//  Synopsis:   Get the max line width of a line inside this table cell.
//
//  Returns:    returns the min of the user specified (if any) width and
//              the width assigned by the table to it. If no width spec'd
//              by the user, then returns the view width.
//
//-------------------------------------------------------------------------

LONG CTableCellLayout::GetMaxLineWidth()
{
    LONG cxWidth = GetSpecifiedPixelWidth(&Doc()->_dci);

    //
    // If user has specified width then cxWidth is returned as non zero
    //
    if (TableLayout() &&
        cxWidth && _iCol >= TableLayout()->_cSizedCols  && 
        !_fInheritedWidth)
    {
        //
        // GetSpecifiedPixelWidth includes the borders and padding. We need
        // to remove these, for the space assigned to them is not available
        // for text layout.
        //
        cxWidth -= GetBorderAndPaddingWidth(&Doc()->_dci);

        //
        // Return the min of the view width and the user specified width.
        // Note that this works for MIN calculations too, when the view
        // width is just 1px, and that is what we will return.
        //
        cxWidth = min(_dp.GetMaxPixelWidth(), cxWidth);
    }
    else
    {
        //
        // If no user specified width, return the view width
        //
        cxWidth = _dp.GetMaxPixelWidth();
    }

    return cxWidth;
}


#ifndef NO_DATABINDING
BOOL CTableCellLayout::IsGenerated()
{
    ELEMENT_TAG etag = Tag();
    return etag != ETAG_CAPTION && 
           etag != ETAG_TC && 
           TableLayout() && 
           TableLayout()->IsGenerated(TableCell()->RowIndex());
}
#endif
