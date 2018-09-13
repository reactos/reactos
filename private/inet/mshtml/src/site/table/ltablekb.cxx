//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltablekb.cxx
//
//  Contents:   CTableLayout keyboard methods.
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif


MtDefine(CTableRowLayoutPageBreak_aCellChoices_pv, Locals, "CTableRowLayout::PageBreak aCellChoices::_pv")

ExternTag(tagPaginate);

//+-------------------------------------------------------------------------
//
//  Method:     GetCellFromPos
//
//  Synopsis:   Retrun the Cell in the table that is closest to the position.
//
//  Arguments:  [ptPosition] - coordinates of the position
//
//  Returns:    table cell, or NULL if the table is empty or the point is
//              outside of the table
//
//--------------------------------------------------------------------------

CTableCell *
CTableLayout::GetCellFromPos(POINT * ptPosition)
{
// BUGBUG: Rewrite this to hit test through the display tree using HT_ELEMENTSWITHLAYOUT
//         and then calling GetNearestFlowLayout on the result (brendand)
// BUGBUG: we should be able to get rid of this function and use DisplayTree to determine 
// the element that is hit by using HitTest (alexa)
    
    CTableCell  *       pCell = NULL;
    CTableCaption   *   pCaption;
    CTableCaption   **  ppCaption;
    int                 yTop, yBottom;
    int                 xLeft, xRight;
    BOOL                fCaptions;
    BOOL                fBelowRows = FALSE;
    CDispNode         * pDispNode;
    int                 cC;
    int                 x, y;
    CTableRowLayout   * pRowLayout = NULL;
    CPoint              pt (*ptPosition);
    
    pDispNode = GetTableInnerDispNode();
    if (!pDispNode)
        return NULL;

    pDispNode->TransformPoint(&pt, COORDSYS_GLOBAL, COORDSYS_CONTAINER);
    if (pt.y >= 0)
    {
        yTop    =
        yBottom = 0;
        pRowLayout = GetRowLayoutFromPos(pt.y, &yTop, &yBottom, &fBelowRows);
    }
    else
    {
        // may be in the top captions
        pt = *ptPosition;
        pDispNode = GetTableOuterDispNode();
        pDispNode->TransformPoint(&pt, COORDSYS_GLOBAL, COORDSYS_CONTAINER);
    }
    
    y = pt.y;
    x = pt.x;

    if (pRowLayout)
    {
        int iCol = GetColExtentFromPos(x, &xLeft, &xRight);
        if (iCol >= 0)
        {
            pCell = Cell(pRowLayout->_aryCells[iCol]);
        }
    }
    else
    {
        fCaptions = _aryCaptions.Size();
        if (fCaptions)
        {
            RECT rcBound;
            // note: the BOTTOM/TOP captions are mixed in the array of captions
            // but the captions are sorted Y position wise
            for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
                 cC > 0;
                 cC--, ppCaption++)
            {
                pCaption = (*ppCaption);
                if ((fBelowRows && pCaption->_uLocation == CTableCaption::CAPTION_BOTTOM) ||
                    (!fBelowRows && pCaption->_uLocation == CTableCaption::CAPTION_TOP))
                {
                    pCell = pCaption;
                    pDispNode = pCaption->Layout()->GetElementDispNode();
                    if (pDispNode)
                    { 
                        pDispNode->GetBounds(&rcBound);
                        if (y < rcBound.bottom)
                        {
                            break;  // we found the caption
                        }
                    }
                }
            }
        }

    }
    return pCell;
}


//+----------------------------------------------------------------------------
//
//  Member:     CTableLayout::RegionFromElement
//
//  Synopsis:   Return the bounding rectangle for a table element, if the element is
//              this instance's owner. The RECT returned is in client coordinates.
//
//  Arguments:  pElement - pointer to the element
//              CDataAry<RECT> *  - rectangle array to contain
//              dwflags - flags define the type of changes required
//              (CLEARFORMATS) etc.
//
//-----------------------------------------------------------------------------
void
CTableLayout::RegionFromElement(CElement       * pElement,
                                CDataAry<RECT> * paryRects,
                                RECT           * prcBound,
                                DWORD            dwFlags)
{
    CRect rcBound;

    Assert( pElement && paryRects);

    if (!pElement || !paryRects)
        return;

    // Clear the array before filling it.
    paryRects->SetSize(0);

    if(!prcBound)
    {
        prcBound = &rcBound;
    }

    *prcBound = g_Zero.rc;

    // If the element passed is the element that owns this instance, let CLayout handle it.
    if (_pElementOwner == pElement)
    {
        super::RegionFromElement(pElement, paryRects, prcBound, dwFlags);
        return;
    }

    CLayout   * pLayout   = pElement->GetCurLayout();
    CDispNode * pDispNode = pLayout ? pLayout->GetElementDispNode() : NULL;
    CDispNode * pGridNode = GetTableInnerDispNode();
    CSize       offsetTableOuterDispNode = (CSize&)g_Zero.size;

    if (!pGridNode)
        return;

    if (!IsTableLayoutCacheCurrent())   // if we are in the middle of modifying the table (and not recacled yet), return
        return;

    // If the element has a displaynode of its own, let it answer itself.
    if (pDispNode)
    {
        pDispNode->GetClientRect(prcBound, CLIENTRECT_CONTENT);
        goto Done;
    }

    // As a start, set the bounding rect to the table grid content area.
    pGridNode->GetClientRect(prcBound, CLIENTRECT_CONTENT);

    switch (pElement->Tag())
    {
    case ETAG_TR:
    {
        CTableRowLayout * pRowLayout = DYNCAST(CTableRowLayout, pElement->GetCurLayout());
        int yTop = 0, yBottom = 0;
        Assert(pRowLayout);

        Verify(GetRowTopBottom(pRowLayout->_iRow, &yTop, &yBottom));
        prcBound->top = yTop;
        prcBound->bottom = yBottom;
        break;
    }
    case ETAG_THEAD:
    case ETAG_TFOOT:
    case ETAG_TBODY:
    {
        CTableSection * pSection = DYNCAST(CTableSection, pElement);
        int yTop = 0, yBottom = 0;
        Assert(pSection);

        if (!pSection->_cRows)
        {
            *prcBound = g_Zero.rc;
            break;
        }

        Verify(GetRowTopBottom(pSection->_iRow, &yTop, &yBottom));
        prcBound->top = yTop;

        Verify(GetRowTopBottom(pSection->_iRow + pSection->_cRows - 1, &yTop, &yBottom));
        prcBound->bottom = yBottom;
        break;
    }
    case ETAG_COLGROUP:
    case ETAG_COL:
    {
        CTableCol * pCol = DYNCAST(CTableCol, pElement);
        int xLeft = 0, xRight = 0;
        Assert(pCol);

        if (!pCol->_cCols || GetCols() == 0 || _aryColCalcs.Size() == 0)
        {
            *prcBound = g_Zero.rc;
            break;
        }

        GetColLeftRight(pCol->_iCol, &xLeft, &xRight);
        prcBound->left = xLeft;
        prcBound->right = xRight;

        // if it is not a spanned column, we are done
        if (pCol->_cCols > 1)
        {
            // if it is a spanned column, get the right coordinate for the last column
            GetColLeftRight(pCol->_iCol + pCol->_cCols - 1, &xLeft, &xRight);
            prcBound->right = xRight;
        }
        break;
    }
    default:
        // Any other elements return empty rect.
        *prcBound = g_Zero.rc;
        break;
    }


    if (_fHasCaptionDispNode)
        pGridNode->GetTransformOffset(&offsetTableOuterDispNode, COORDSYS_CONTENT, COORDSYS_PARENT);

    OffsetRect(prcBound, offsetTableOuterDispNode.cx, offsetTableOuterDispNode.cy);

Done:

    if (dwFlags & RFE_SCREENCOORD)
    {
        TransformRect(prcBound, COORDSYS_CONTENT, COORDSYS_GLOBAL);
    }

    if (!IsRectEmpty(prcBound))
        paryRects->AppendIndirect(prcBound);
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableLayout::GetRowTopBottom
//
//  Synopsis:   Obtain the row's y-extent.
//
//  Arguments:  [iRowLocate]  - IN:  index of row to be located
//              [pyRowTop]    - OUT: yTop of row
//              [pyRowBottom] - OUT: yBottom of row
//
//  Note:       Cellspacing is considered OUTSIDE the row.
//
//  Returns:    TRUE if row found.
//
//--------------------------------------------------------------------------

BOOL
CTableLayout::GetRowTopBottom(int iRowLocate, int * pyRowTop, int * pyRowBottom)
{
    CTableRowLayout * pRowLayoutLocate, * pRowLayout;
    int               iRow, cRows = GetRows(), iRowLast;

    Assert(pyRowTop && pyRowBottom);

    if (iRowLocate < 0 || iRowLocate >= cRows)
    {
        *pyRowTop = *pyRowBottom = 0;
        return FALSE;
    }

    pRowLayoutLocate = _aryRows[iRowLocate]->Layout();
    Assert(pRowLayoutLocate);

    iRow = GetFirstRow();
    iRowLast = GetLastRow();
    pRowLayout = _aryRows[iRow]->Layout();
    *pyRowTop = _yCellSpacing;
    *pyRowBottom = pRowLayout->_yHeight + _yCellSpacing;

    while (iRowLocate != iRow && iRow != iRowLast)
    {
        iRow = GetNextRowSafe(iRow);
        pRowLayout = _aryRows[iRow]->Layout();
        *pyRowTop = *pyRowBottom + _yCellSpacing;
        *pyRowBottom += pRowLayout->_yHeight + _yCellSpacing;
    }

    Assert(iRowLocate == iRow);

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableLayout::GetColLeftRight
//
//  Synopsis:   Obtain the column's x-extent.
//
//  Arguments:  [iColLocate] - IN:  index of col to be located
//              [pxColLeft]  - OUT: xLeft of col
//              [pxColRight] - OUT: xRight of col
//
//  Note:       Cellspacing is considered OUTSIDE the column.
//
//  Returns:    TRUE if column found.
//
//--------------------------------------------------------------------------

BOOL
CTableLayout::GetColLeftRight(int iColLocate, int * pxColLeft, int * pxColRight)
{
    int iCol = 0, cCols = GetCols(), xWidth;

    Assert (pxColLeft  && pxColRight);

    *pxColLeft = *pxColRight = 0;

    if (_aryColCalcs.Size() != cCols) 
    {
        Assert (FALSE && "we should not reach this code, since we have to be calced at this point");
        return FALSE;
    }

    if (iColLocate < 0 || iColLocate >= cCols)
    {
        return FALSE;
    }

    // NOTE: This code works for RTL as well.
    *pxColLeft  = _xCellSpacing;
    *pxColRight = _aryColCalcs[0]._xWidth + _xCellSpacing;

    while (iColLocate != iCol && iCol < cCols-1)
    {
        xWidth = _aryColCalcs[++iCol]._xWidth;
        *pxColLeft = *pxColRight + _xCellSpacing;
        *pxColRight += xWidth + _xCellSpacing;
    }

    Assert(iColLocate == iCol);

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableLayout::GetRowLayoutFromPos
//
//  Synopsis:   Returns the row layout in the table at the y-position and its
//              y-extent.
//
//  Arguments:  [y]           - IN:  y-coordinate of the position
//              [pyRowTop]    - OUT: yTop of row
//              [pyRowBottom] - OUT: yBottom of row
//
//  Returns:    Table row layout, or NULL if the table is empty or y-position
//              is outside the table.  Bottom cellspacing is considered part of
//              the row (i.e. the top cellspacing corresponds to no row).
//
//--------------------------------------------------------------------------

CTableRowLayout *
CTableLayout::GetRowLayoutFromPos(int y, int * pyRowTop, int * pyRowBottom, BOOL *pfBelowRows)
{
    CTableRowLayout   * pRowLayout = NULL;
    int                 iRow, cRows = GetRows(), iRowLast;

    Assert(pyRowTop && pyRowBottom);

    *pyRowTop = 0;
    *pyRowBottom = 0;

    if (pfBelowRows)
    {
        *pfBelowRows = FALSE;
    }

    if (!cRows || y<0)
        goto Cleanup;

    iRow          = GetFirstRow();
    iRowLast      = GetLastRow();
    pRowLayout    = _aryRows[iRow]->Layout();
    *pyRowTop    += _yCellSpacing;
    *pyRowBottom += pRowLayout->_yHeight + 2*_yCellSpacing; // two cellspacings for first row: above AND below

    while (y >= *pyRowBottom && iRow != iRowLast)
    {
        iRow = GetNextRowSafe(iRow);
        pRowLayout = _aryRows[iRow]->Layout();
        *pyRowTop = *pyRowBottom;
        *pyRowBottom += pRowLayout->_yHeight + _yCellSpacing;
    }

    if (pfBelowRows)
    {
        *pfBelowRows = TRUE;
    }

Cleanup:

    return pRowLayout;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableLayout::GetColExtentFromPos
//
//  Synopsis:   Returns the x-extent of the column in the table at the
//              specified x-position.
//
//  Arguments:  [x]          - IN:  x-coordinate of the position
//              [pxColLead]  - OUT: xLead of column (side on which column begins - 
//                                  left for Left To Right and right for Right To Left)
//              [pyColTrail] - OUT: xTrail of column (side on which column ends -
//                                  right for Left To Right and left for Right To Left)
//              [fRightToLeft] - IN: the table is layed out RTL (origin it top/right)
//
//  Returns:    Column index found located between [*pxColLeft,*pxColRight).
//              Else returns -1.  Uses colcalcs.
//
//--------------------------------------------------------------------------

int
CTableLayout::GetColExtentFromPos(int x, int * pxColLead, int * pxColTrail, BOOL fRightToLeft)
{
    int iCol = 0, cCols = GetCols(), xWidth;

    if (!cCols || (!fRightToLeft ? x<0 : x>0))
        return -1;

    *pxColLead  = _xCellSpacing;

    if(!fRightToLeft)
    {
        *pxColTrail = _aryColCalcs[0]._xWidth + 2*_xCellSpacing;

        while (x >= *pxColTrail && iCol < cCols-1)
        {
            xWidth = _aryColCalcs[++iCol]._xWidth;
            *pxColLead = *pxColTrail;
            *pxColTrail += xWidth + _xCellSpacing;
        }
    }
    else
    {
        *pxColTrail = - _aryColCalcs[0]._xWidth - 2*_xCellSpacing;
        
        while (x <= *pxColTrail && iCol < cCols-1)
        {
            xWidth = _aryColCalcs[++iCol]._xWidth;
            *pxColLead = *pxColTrail;
            *pxColTrail -= xWidth + _xCellSpacing;
        }
    }

    return iCol;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTableLayout::GetHeaderFooterRects
//
//  Synopsis:   Retrieves the global rect of THEAD and TFOOT.
//
//  Arguments:  [prcTableHeader] - OUT: global rect of header
//              [prcTableFooter] - OUT: global rect of footer
//
//  Returns:    Rects if header and footer exist AND are supposed to be
//              repeated.  Empty rects otherwise.
//
//--------------------------------------------------------------------------

void
CTableLayout::GetHeaderFooterRects(RECT * prcTableHeader, RECT * prcTableFooter)
{
    CDispNode * pElemDispNode = GetElementDispNode();
    CDispNode * pDispNode = GetTableInnerDispNode();
    CTableRowLayout * pRowLayout;
    CRect rc, rcInner;
    CSize sizeGlobalOffset;
    int iRow;

    if (!pDispNode)
        return;

    Assert(prcTableHeader && prcTableFooter);
    memset(prcTableHeader, 0, sizeof(RECT));
    memset(prcTableFooter, 0, sizeof(RECT));

    // GetBounds returns coordinates in PARENT system.
    pElemDispNode->GetBounds(&rc);
    pElemDispNode->GetTransformOffset(&sizeGlobalOffset, COORDSYS_PARENT, COORDSYS_GLOBAL);
    rc.OffsetRect(sizeGlobalOffset.cx, sizeGlobalOffset.cy);
    rc.left = 0;

    pDispNode->GetClientRect(&rcInner, CLIENTRECT_CONTENT);
    pDispNode->GetTransformOffset(&sizeGlobalOffset, COORDSYS_CONTENT, COORDSYS_GLOBAL);
    rcInner.OffsetRect(sizeGlobalOffset.cx, sizeGlobalOffset.cy);

    if (_pHead && _pHead->_cRows)
    {
        CTreeNode * pNode = _pHead->GetFirstBranch();
        const CFancyFormat * pFF = pNode ? pNode->GetFancyFormat() : NULL;

        // If repeating of table headers is set on THEAD, calculate the rect.
        if (pFF && pFF->_bDisplay == styleDisplayTableHeaderGroup)
        {
            *prcTableHeader = rc;
            prcTableHeader->top = rcInner.top + _yCellSpacing;
            prcTableHeader->bottom = prcTableHeader->top;

            for ( iRow = _pHead->_iRow ; iRow < _pHead->_iRow + _pHead->_cRows ; iRow++ )
            {
                pRowLayout = GetRow(iRow)->Layout();
                Assert(pRowLayout);

                prcTableHeader->bottom += _yCellSpacing;
                prcTableHeader->bottom += pRowLayout->_yHeight;
            }
        }
    }

    if (_pFoot && _pFoot->_cRows)
    {
        CTreeNode * pNode = _pFoot->GetFirstBranch();
        const CFancyFormat * pFF = pNode ? pNode->GetFancyFormat() : NULL;

        // If repeating of table footers is set on TFOOT, calculate the rect.
        if (pFF && pFF->_bDisplay == styleDisplayTableFooterGroup)
        {
            *prcTableFooter = rc;
            prcTableFooter->bottom = rcInner.bottom - _yCellSpacing;
            prcTableFooter->top = prcTableFooter->bottom;

            for ( iRow = _pFoot->_iRow ; iRow < _pFoot->_iRow + _pFoot->_cRows ; iRow++ )
            {
                pRowLayout = GetRow(iRow)->Layout();
                Assert(pRowLayout);

                prcTableFooter->top -= pRowLayout->_yHeight;
                prcTableFooter->top -= _yCellSpacing;
            }
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     GetNextFlowLayout
//
//  Synopsis:   Retrun a cell (if it exists) in the direction specified by
//              iDir.
//
//  Arguments:  [iDir]:       The direction to search in
//              [ptPosition]: The point from where to start looking for a cell
//              [pElementLayout]: The child site from where this call came and to which
//                            the point belongs.
//              [pcp]:        The cp in the new txt site
//              [pfCaretNotAtBOL]: Is the caret at BOL?
//              [pfAtLogicalBOL] : Is the caret at logical BOL?
//
//  Returns:    The txtsite in the direction specified if one exists,
//              NULL otherwise.
//
//--------------------------------------------------------------------------

CFlowLayout *
CTableLayout::GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp,
                                BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL)
{
    CFlowLayout *pFlowLayout;

    if (IsCaption(pElementLayout->Tag()))
    {
        pFlowLayout = GetNextFlowLayoutFromCaption(iDir, ptPosition, DYNCAST(CTableCaption, pElementLayout));
    }
    else
    {
        CTableCell *pCell = DYNCAST(CTableCell, pElementLayout);
        int iCol = pCell->Layout()->_iCol;

        if (iDir == NAVIGATE_UP || iDir == NAVIGATE_DOWN)
        {
            iCol += pCell->ColSpan() - 1;    // if cell is spanned
        }

        pFlowLayout = GetNextFlowLayoutFromCell(iDir, ptPosition, pCell->RowIndex(), iCol);
    }

    // If we find a table cell then lets find the position we want to be at
    // in that table cell. If we did not find a cell, then pass this call
    // to our parent.
    return pFlowLayout
            ? pFlowLayout->GetPositionInFlowLayout(iDir, ptPosition, pcp, pfCaretNotAtBOL, pfAtLogicalBOL)
            : GetUpdatedParentLayout()->GetNextFlowLayout(iDir, ptPosition, Table(), pcp, pfCaretNotAtBOL, pfAtLogicalBOL);
}


//+-------------------------------------------------------------------------
//
//  Method:     GetNextFlowLayout
//
//  Synopsis:   Retrun a cell (if it exists) in the direction specified by
//              iDir.
//
//  Arguments:  [iDir]:       The direction to search in
//              [ptPosition]: The point from where to start looking for a cell
//              [pElementLayout]: The child element (with layout) from where this
//                            call came and to which the point belongs.
//              [pcp]:        The cp in the new txt site
//              [pfCaretNotAtBOL]: Is the caret at BOL?
//              [pfAtLogicalBOL] : Is the caret at logical BOL?
//
//  Returns:    The txtsite in the direction specified if one exists,
//              NULL otherwise.
//
//  Note:       This method just routes the call straight up to the table and
//              passes the incoming child as the child rather than itself.
//
//--------------------------------------------------------------------------

CFlowLayout *
CTableRowLayout::GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp,
                                   BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL)
{
    return TableLayout()->GetNextFlowLayout(iDir, ptPosition, pElementLayout, pcp, pfCaretNotAtBOL, pfAtLogicalBOL);
}


//+----------------------------------------------------------------------------
//
//  Member:     GetNextFlowLayoutFromCell
//
//  Synopsis:   Get next text site in the specified direction from the
//              specified position
//
//  Arguments:  [iDir]       -  UP/DOWN/LEFT/RIGHT
//              [ptPosition] -  position in the current txt site
//              [iRow]       -  (current) cell's row number
//              [iCol]       -  (current) cell's column number
//
//-----------------------------------------------------------------------------

CFlowLayout *
CTableLayout::GetNextFlowLayoutFromCell(NAVIGATE_DIRECTION iDir, POINT ptPosition, int iRow, int iCol)
{
    CPoint ptContent(ptPosition);
    TransformPoint(&ptContent, COORDSYS_GLOBAL, COORDSYS_CONTENT);
    
    if (ptContent.x < 0 && (iDir == NAVIGATE_UP || iDir == NAVIGATE_DOWN))
    {
        ptContent.x = 0;
    }

    int                 x = ptContent.x;
    CTableCaption   *   pCaption;
    CTableCaption   **  ppCaption;
    int                 cC;
    int                 cRows;
    int                 cCols;
    BOOL                fGoingUp = FALSE;
    CFlowLayout     *   pFlowLayout = NULL;
    CTableCell      *   pCell = NULL;

    if (EnsureTableLayoutCache())
        return NULL;

    
    cRows = GetRows();
    cCols = GetCols();

    //  Note:       iRow could be outside of the _AryRows range by +/-1
    //              iCol could be outside of the _aryColumns range by +/-1
    Assert (iRow >= -1 && iRow <= cRows);
    Assert (iCol >= -1 && iCol <= cCols);

    switch (iDir)
    {
    case NAVIGATE_LEFT:
        while (iRow >= 0 && !pCell)
        {
            while (--iCol >= 0 && !pCell)
            {
                // go to the cell on the left
                pCell = GetCellBy(iRow, iCol);
            }
            // go to the right most cell of the previous row
            iRow = GetPreviousRowSafe(iRow);
            iCol = cCols;
        }
        fGoingUp = TRUE;
        break;

    case NAVIGATE_RIGHT:
        while (iRow < cRows && !pCell)
        {
            while (++iCol < cCols && !pCell)
            {
                // go to the cell on the right
                pCell = GetCellBy(iRow, iCol);

                // If we ended up in the middle of a colspan, walk out of that cell in next iteration.
                // This case is taken care of inside GetCellBy.
            }
            // go to the left most cell of the next row
            iRow = GetNextRowSafe(iRow);
            iCol = -1;
        }
        break;

    case NAVIGATE_UP:
        while ((iRow = GetPreviousRowSafe(iRow)) >= 0 && !pCell)
        {
            AssertSz (IsReal(pCell), "We found a row/colspan");
            pCell = GetCellBy(iRow, iCol, x);
        }
        fGoingUp = TRUE;
        break;

    case NAVIGATE_DOWN:
        while ((iRow = GetNextRowSafe(iRow)) < cRows && !pCell)
        {
            AssertSz (IsReal(pCell), "We found a row/colspan");
            pCell = GetCellBy(iRow, iCol, x);

            // If we ended up in the middle of a rowspan, walk out of that cell in next iteration.
            if (pCell && pCell->RowSpan() > 1 && iRow > pCell->RowIndex())
            {
                Assert(pCell->RowIndex() + pCell->RowSpan() - 1 >= iRow);
                iRow = pCell->RowIndex() + pCell->RowSpan() - 1;
                pCell = NULL;
            }
        }
        break;

    }

    // Get the layout of the real cell.
    if (pCell)
    {
        AssertSz(IsReal(pCell), "We need to have a real cell");
        pFlowLayout = pCell->Layout();
    }

    if (!pFlowLayout && _aryCaptions.Size())
    {
        // note: the BOTTOM/TOP captions are mixed in the array of captions
        // but the captions are sorted Y position wise
        for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
             cC > 0;
             cC--, ppCaption++)
        {
            pCaption = (*ppCaption);
            if (!pCaption->Layout()->NoContent())
            {
                if (fGoingUp)
                {
                    // So we need to take last TOP caption
                    if (pCaption->_uLocation == CTableCaption::CAPTION_TOP)
                    {
                        pFlowLayout = DYNCAST(CFlowLayout, pCaption->GetCurLayout());
                    }
                }
                else
                {
                    // So we need to take first BOTTOM caption
                    if (pCaption->_uLocation == CTableCaption::CAPTION_BOTTOM)
                    {
                        pFlowLayout = DYNCAST(CFlowLayout, pCaption->GetCurLayout());
                        break;
                    }
                }
            }
        }
    }

    return pFlowLayout;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetNextFlowLayoutFromCaption
//
//  Synopsis:   Get next text site in the specified direction from the
//              specified position
//
//  Arguments:  [iDir]       -  UP/DOWN/LEFT/RIGHT
//              [ptPosition] -  position in the current txt site
//              [pCaption]   -  current caption
//
//-----------------------------------------------------------------------------

CFlowLayout *
CTableLayout::GetNextFlowLayoutFromCaption(NAVIGATE_DIRECTION iDir, POINT ptPosition, CTableCaption *pCaption)
{
    unsigned    uLocation = pCaption->_uLocation;   // Caption placing (TOP/BOTTOM)
    int         i, cC, iC;

    if( EnsureTableLayoutCache() )
        return NULL;

    cC = _aryCaptions.Size();
    iC = _aryCaptions.Find(pCaption);
    Assert (iC >=0 && iC < cC);

    switch (iDir)
    {
    case NAVIGATE_LEFT:
    case NAVIGATE_UP:
        for (i = iC - 1; i >= 0; i--)
        {
            pCaption = _aryCaptions[i];
            if (!pCaption->Layout()->NoContent())
            {
                if (pCaption->_uLocation == uLocation)
                {
                    return DYNCAST(CFlowLayout, pCaption->GetCurLayout());
                }
            }
        }
        if (uLocation == CTableCaption::CAPTION_BOTTOM)
        {
            return GetNextFlowLayoutFromCell(iDir, ptPosition, GetRows(), 0);
        }
        break;
    case NAVIGATE_RIGHT:
    case NAVIGATE_DOWN:
        for (i = iC + 1; i < cC; i++)
        {
            pCaption = _aryCaptions[i];
            if (!pCaption->Layout()->NoContent())
            {
                if (pCaption->_uLocation == uLocation)
                {
                    return DYNCAST(CFlowLayout, pCaption->GetCurLayout());
                }
            }
        }
        if (uLocation == CTableCaption::CAPTION_TOP)
        {
            return GetNextFlowLayoutFromCell(iDir, ptPosition, -1, GetCols());
        }
        break;

    }

    return NULL;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetCellBy
//
//  Synopsis:   get the cell from the specified row and from the specified column
//              that is positioned under the specified X-position.
//
//  Arguments:  iRow     - row
//              iCol     - column
//              X        - x position
//
//-----------------------------------------------------------------------------

CTableCell *
CTableLayout::GetCellBy(int iRow, int iCol, int x)
{
    CTableCell  *pCell = NULL;
    CTableRow   *pRow;

    Assert(IsTableLayoutCacheCurrent());

    int xLeft, xRight;
    iCol = GetColExtentFromPos(x, &xLeft, &xRight);

    while (iCol >= 0)
    {
        pRow = _aryRows[iRow];
        pCell = pRow->Layout()->_aryCells[iCol];
        if (pCell && IsReal(pCell))
        {
            break;
        }
        // go to the cell on the left
        iCol--; // colSpan case
    }
    return Cell(pCell);
}


//+----------------------------------------------------------------------------
//
//  Member:     GetCellBy
//
//  Synopsis:   get the cell from the specified row and from the specified column
//
//  Arguments:  iRow     - row
//              iCol     - column
//
//-----------------------------------------------------------------------------

inline CTableCell *
CTableLayout::GetCellBy(int iRow, int iCol)
{
    Assert(IsTableLayoutCacheCurrent());
    CTableRow *pRow = _aryRows[iRow];
    return Cell(pRow->Layout()->_aryCells[iCol]);
}


//+----------------------------------------------------------------------------
//
//  Member:     CTableLayout::PageBreak
//
//  Synopsis:   Page-Break the table at lyPos. Provide the reasonable choices for
//              the pagenator.
//
//  Arguments:  lyStart  - starting position (don't return anything above it)
//              lyPos    -  Y position for the desired page break
//              paryChoices -  pointer to the array of choices (results)
//
//  Returns:    S_OK for now
//
//-----------------------------------------------------------------------------

HRESULT
CTableLayout::PageBreak(long lyStart, long lyPos, CStackPageBreaks * paryChoices)
{
    CTableRowLayout *   pRowLayout;
    CDispNode *         pDispNode = GetTableInnerDispNode();
    CRect               rc;
    CSize               sizeOffset;
    long                lyStartLocal, lyEndLocal;
    int                 yRowTop = 0, yRowBottom = 0;
    int                 iRow, iRowTop, iRowBottom, cRows = _aryRows.Size();
    HRESULT             hr = S_OK;

    if (!pDispNode)
        goto Cleanup;

    // Parameter check.
    Assert(lyStart < lyPos);

    pDispNode->GetClientRect(&rc, CLIENTRECT_CONTENT);
    pDispNode->GetTransformOffset(&sizeOffset, COORDSYS_CONTENT, COORDSYS_GLOBAL);
    rc.OffsetRect(sizeOffset.cx, sizeOffset.cy);
    lyStartLocal = lyStart - rc.top;
    lyEndLocal = lyPos - rc.top;

    if (lyEndLocal <= 0 || lyStartLocal >= rc.bottom)
        goto BreakCaptions;

    // Optimization: Consult only rows that overlap yRange.
    pRowLayout = GetRowLayoutFromPos(lyStartLocal, &yRowTop, &yRowBottom);
    iRowTop = pRowLayout ? pRowLayout->_iRow : GetFirstRow();
    pRowLayout = GetRowLayoutFromPos(lyEndLocal, &yRowTop, &yRowBottom);
    iRowBottom = pRowLayout ? pRowLayout->_iRow : GetLastRow();
    
    for (iRow = iRowTop ; iRow < cRows ; iRow = GetNextRowSafe(iRow))
    {
        pRowLayout = GetRow(iRow)->Layout();
        if (pRowLayout)
        {
            hr = pRowLayout->PageBreak(lyStart, lyPos, paryChoices);
            if (hr)
                goto Cleanup;
        }

        // Finished last row?
        if (iRow == iRowBottom)
            break;
    }


BreakCaptions:

    // With captions we are lazy and ask all of them (flowlayout sufficiently optimizes this).
    if (_aryCaptions.Size())
    {
        CTableCaption * pCaption, ** ppCaption;
        CFlowLayout *   pFlowLayout;
        int             cC;

        for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
             cC > 0;
             cC--, ppCaption++)
        {
            pCaption = (*ppCaption);
            if (!pCaption->Layout()->NoContent())
            {
                // So we need to take last TOP caption
                pFlowLayout = DYNCAST(CFlowLayout, pCaption->GetCurLayout());
                pFlowLayout->PageBreak(lyStart, lyPos, paryChoices);
            }
        }
    }

Cleanup:

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     CTableLayout::ContainsPageBreak
//
//  Synopsis:   Checks whether table has page-break-before or nested table
//              element sets page-break-before or -after.
//
//  Arguments:  yTop    - top position to look for
//              yBottom - bottom position to look for
//              pyBreak - OUT: pagebreak position found
//
//  Returns:    S_OK if a pagebreak was found - else S_FALSE.
//
//-----------------------------------------------------------------------------

HRESULT
CTableLayout::ContainsPageBreak(long yTop, long yBottom, long * pyBreak, BOOL * pfPageBreakAfterAtTopLevel)
{
    CTableRowLayout    * pRowLayout;
    CTableCell         * pCell;
    CTableCellLayout   * pCellLayout;
    CTableSection      * pSection;
    CDispNode          * pDispNodeGrid = GetTableInnerDispNode(), * pDispNode;
    BOOL                 fPageBreakBefore = HasPageBreakBefore();
    BOOL                 fPageBreakAfter, fPageBreakAfterTable = HasPageBreakAfter();
    BOOL                 fTableCellBreak = FALSE;
    int                  iRow, iRowTop, iRowBottom, iCol, iColSpan;
    int                  yRowTop, yRowBottom;
    long                 yBreakWalk = 0, yGridOffset, yFirstCellBreak = yBottom;
    SIZE                 sizeDispNode;
    RECT                 rcGridOffset;
    HRESULT              hr = S_FALSE;

    Assert(pyBreak);

    // If the vertical slice is non-existent (empty), we don't have a pagebreak.  Return S_FALSE.
    if (yTop > yBottom)
        goto Cleanup;

    //
    // 1. Check if table has a before break.
    //

    if (fPageBreakBefore && yTop <= 0)
        goto FoundNonGridBreak;

    //
    // 2. Check top captions.
    //

    if ( THR(ContainsCaptionPageBreak(TRUE, 0, yTop, yBottom, &yBreakWalk)) == S_OK )
        goto FoundNonGridBreak;

    //
    // 3. Check rows in visual order.
    //

    // Subtract offset since all rows are inside the table borders.

    if (!pDispNodeGrid)
        goto Cleanup;

    pDispNodeGrid->GetBorderWidths(&rcGridOffset);
    yGridOffset = rcGridOffset.top;
    if (_fHasCaptionDispNode)
    {
        pDispNodeGrid->GetBounds(&rcGridOffset);
        yGridOffset += rcGridOffset.top;
    }

    yTop -= yGridOffset;
    yBottom -= yGridOffset;

    // Determine top row.
    yRowTop    =
    yRowBottom = 0;
    pRowLayout = GetRowLayoutFromPos(yTop, &yRowTop, &yRowBottom);
    iRowTop = pRowLayout ? pRowLayout->_iRow : GetFirstRow();
    yBreakWalk = pRowLayout ? yRowTop : _yCellSpacing;

    // Determine bottom row.
    yRowTop    =
    yRowBottom = 0;
    pRowLayout = GetRowLayoutFromPos(yBottom, &yRowTop, &yRowBottom);
    iRowBottom = pRowLayout ? pRowLayout->_iRow : GetLastRow();

    for ( iRow = iRowTop ; ; iRow = GetNextRowSafe(iRow) )
    {
        pRowLayout = GetRow(iRow)->Layout();
        Assert(pRowLayout);
        pSection = pRowLayout->Section();

        // Obtain row formats.
        fPageBreakBefore = pRowLayout->HasPageBreakBefore();
        fPageBreakAfter = pRowLayout->HasPageBreakAfter();

        // If row first in section, inherit section's before-break.
        fPageBreakBefore |= pSection && pSection->_iRow == pRowLayout->_iRow && pSection->HasPageBreakBefore();

        // Handle row's break-before.
        if (fPageBreakBefore && yBreakWalk >= yTop && yBreakWalk <= yBottom)
            goto FoundBreak;

        //
        // Check for table cell breaks.
        //

        for ( iCol = 0 ; iCol < GetCols() ; iCol += iColSpan )
        {
            long yCellBreak;

            pCell = pRowLayout->_aryCells[iCol];
            if (IsReal(pCell))
            {
                Assert(pCell && pCell == Cell(pCell));
                pCellLayout = Cell(pCell)->Layout();

                if (   THR(pCellLayout->ContainsPageBreak(
                            (yTop > yBreakWalk) ? (yTop - yBreakWalk) : 0,
                            yBottom - yBreakWalk,
                            &yCellBreak)) == S_OK
                    && yCellBreak + yBreakWalk >= yTop
                    && yCellBreak + yBreakWalk <= yBottom )
                {
                    // At this point, we know cells are going to pagebreak this row.
                    // We still have to make sure though that we let the first one
                    // through.
                    fTableCellBreak = TRUE;

                    if (yFirstCellBreak > yBreakWalk + yCellBreak)
                        yFirstCellBreak = yBreakWalk + yCellBreak;
                }

                iColSpan = pCell->ColSpan();
            }
            else
            {
                iColSpan = 1;
            }
        }

        // Process topmost tablecell break.
        if (fTableCellBreak)
        {
            yBreakWalk = yFirstCellBreak;
            goto FoundBreak;
        }

        // Jump to end of row.
        yBreakWalk += pRowLayout->_yHeight;

        // If row last in section, inherit section's after-break.
        fPageBreakAfter |= pSection && pSection->_iRow + pSection->_cRows - 1 == pRowLayout->_iRow && pSection->HasPageBreakAfter();

        if (fPageBreakAfter && yBreakWalk >= yTop && yBreakWalk <= yBottom)
            goto FoundBreak;

        yBreakWalk += _yCellSpacing;

        // Finished last row?
        if (iRow == iRowBottom)
            break;
    }

    //
    // 4. Check bottom captions.
    //

    // Leaving table grid.  Remove offset.
    yTop += yGridOffset;
    yBottom += yGridOffset;

    Assert(!_aryCaptions.Size() || _fHasCaptionDispNode);
    if ( THR(ContainsCaptionPageBreak(FALSE, rcGridOffset.bottom, yTop, yBottom, &yBreakWalk)) == S_OK )
        goto FoundNonGridBreak;

    //
    // 5. Check if table has a page-break-after.
    //

    if (fPageBreakAfterTable)
    {
        // Find out if the bottom of this layout is above yBottom.
        pDispNode = GetElementDispNode();
        if (!pDispNode)
            goto Cleanup;

        pDispNode->GetSize(&sizeDispNode);
        if (sizeDispNode.cy <= yBottom)
        {
            yBreakWalk = sizeDispNode.cy;

            if (pfPageBreakAfterAtTopLevel)
                *pfPageBreakAfterAtTopLevel = TRUE;

            goto FoundNonGridBreak;
        }
    }

    //
    // No pagebreak found in range.
    //

Cleanup:

    RRETURN1(hr, S_FALSE);

FoundBreak:

    yBreakWalk += yGridOffset;

FoundNonGridBreak:

    *pyBreak = yBreakWalk;

    hr = S_OK;
    goto Cleanup;
}


//+----------------------------------------------------------------------------
//
//  Member:     CTableLayout::ContainsCaptionPageBreak
//
//  Synopsis:   Helper function.  Checks for pagebreaks in table captions.
//
//  Arguments:  fTopCaptions - CAPTION_TOP or CAPTION_BOTTOM
//              yBreakWalk   - Y Position of sweep line
//              yTop         - top position to look for
//              yBottom      - bottom position to look for
//              pyBreak      - OUT: pagebreak position found
//
//  Returns:    S_OK if a pagebreak was found - else S_FALSE.
//
//-----------------------------------------------------------------------------

HRESULT
CTableLayout::ContainsCaptionPageBreak(BOOL fTopCaptions, long yBreakWalk, long yTop, long yBottom, long * pyBreak)
{
    CTableCaption    * pCaption, ** ppCaption;
    CTableCellLayout * pCaptionLayout;
    CDispNode        * pDispNode;
    SIZE               sizeDispNode;
    long               yCaptionBreak;
    int                cC;
    HRESULT            hr = S_FALSE;

    Assert(pyBreak);

    if (_aryCaptions.Size())
    {
        // Note: The top/bottom captions are mixed in the array of captions
        // but the captions are sorted Y position wise.
        for ( cC = _aryCaptions.Size(), ppCaption = _aryCaptions ; cC > 0 ; cC--, ppCaption++ )
        {
            pCaption = (*ppCaption);
            pCaptionLayout = pCaption->Layout();

            if (   !pCaptionLayout->NoContent()
                && (fTopCaptions && pCaption->_uLocation == CTableCaption::CAPTION_TOP)
                 || (!fTopCaptions && pCaption->_uLocation == CTableCaption::CAPTION_BOTTOM))
            {
                if (   THR(pCaptionLayout->ContainsPageBreak(
                            (yTop > yBreakWalk) ? (yTop - yBreakWalk) : 0,
                            yBottom - yBreakWalk,
                            &yCaptionBreak)) == S_OK
                    && yCaptionBreak + yBreakWalk >= yTop
                    && yCaptionBreak + yBreakWalk <= yBottom )
                {
                    yBreakWalk += yCaptionBreak;
                    goto FoundBreak;
                }

                pDispNode = pCaptionLayout->GetElementDispNode();
                if (pDispNode)
                {
                    pDispNode->GetSize(&sizeDispNode);
                    yBreakWalk += sizeDispNode.cy;
                }
            }
        }
    }

    // No caption break found.

Cleanup:

    RRETURN1(hr, S_FALSE);

FoundBreak:

    *pyBreak = yBreakWalk;
    hr = S_OK;
    goto Cleanup;
}


//+----------------------------------------------------------------------------
//
//  Member:     CTableRowLayout::PageBreak
//
//  Synopsis:   Page-Break the table at lyPos. Provide the reasonable choices for
//              the pagenator.
//
//  Arguments:  lyStart  - starting position (don't return anything above it)
//              lyPos    -  Y position for the desired page break
//              pChoices -  pointer to the array of choices (results)
//
//  Returns:    S_OK for now
//
//-----------------------------------------------------------------------------

HRESULT
CTableRowLayout::PageBreak(long lyStart, long lyPos, CStackPageBreaks *paryChoices)
{
    CTableCell * pCell, ** ppCell;
    int          cColSpan, cC;
    HRESULT      hr = S_OK;

    for (cC = _aryCells.Size(), ppCell = _aryCells;
        cC > 0 ; cC -= cColSpan, ppCell += cColSpan)
    {
        pCell = Cell(*ppCell);
        if (pCell)
        {
            CTableCellLayout *pCellLayout = DYNCAST(CTableCellLayout, pCell->GetCurLayout());

            hr = pCellLayout->PageBreak(lyStart, lyPos, paryChoices);
            if (hr)
                goto Cleanup;

            cColSpan = pCell->ColSpan();
        }
        else
        {
            cColSpan = 1;
        }
    }

Cleanup:

    RRETURN(hr);
}
