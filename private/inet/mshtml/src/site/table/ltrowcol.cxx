//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltrow.cxx
//
//  Contents:   Implementation of CTableCellLayout and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

MtDefine(CTableRowLayout, Layout, "CTableRowLayout")
MtDefine(CTableRowLayout_aryCells_pv, CTableRowLayout, "CTableRowLayout::_aryCells::_pv")
MtDefine(CTableRowLayout_pDisplayNoneCells_pv, CTableRowLayout, "CTableRowLayout::_pDisplayNoneCells::_pv")

const CLayout::LAYOUTDESC CTableRowLayout::s_layoutdesc =
{
    0,                              // _dwFlags
};

//+------------------------------------------------------------------------
//
//  Member:     CTableRowLayout::destructor
//
//  Note:       The display none cache must be deleted in the destructor
//
//-------------------------------------------------------------------------

CTableRowLayout::~CTableRowLayout()
{
    if (_pDisplayNoneCells)
    {
        _pDisplayNoneCells->DeleteAll();
        delete _pDisplayNoneCells;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     GetFirstLayout
//
//  Synopsis:   Enumeration method to loop thru children (start)
//
//  Arguments:  [pdw]       cookie to be used in further enum
//              [fBack]     go from back
//
//  Returns:    site
//
//  Note:       If you change the way the cookie is implemented, then
//              please also change the GetCookieForSite funciton.
//
//-------------------------------------------------------------------------

CLayout *
CTableRowLayout::GetFirstLayout(DWORD_PTR * pdw, BOOL fBack, BOOL fRaw)
{
    // BUGBUG EricVas: Implement fRaw
    //        Don't have to until we get table editing
    *pdw = fBack ? (DWORD)_aryCells.Size() : (DWORD)-1;
    return CTableRowLayout::GetNextLayout(pdw, fBack);
}


//+------------------------------------------------------------------------
//
//  Member:     GetNextLayout
//
//  Synopsis:   Enumeration method to loop thru children
//
//  Arguments:  [pdw]       cookie to be used in further enum
//              [fBack]     go from back
//
//  Returns:    site
//
//-------------------------------------------------------------------------

CLayout *
CTableRowLayout::GetNextLayout(DWORD_PTR * pdw, BOOL fBack, BOOL fRaw)
{
    // BUGBUG EricVas: Implement fRaw
    //        Don't have to until we get table editing
    int i;
    CTableCell * pCell;

    for (;;)
    {
        i = (int)*pdw;
        if (fBack)
        {
            if (i > 0)
            {
                i--;
            }
            else
            {
                return NULL;
            }
        }
        else
        {
            if (i < _aryCells.Size() - 1)
            {
                i++;
            }
            else
            {
                return NULL;
            }
        }
        *pdw = (DWORD)i;
        pCell = _aryCells[i];
        if (IsReal(pCell))
        {
            return pCell->GetCurLayout();
        }
    }
}

//+---------------------------------------------------------------------------
//
// Member:      ContainsChildLayout
//
//----------------------------------------------------------------------------
BOOL
CTableRowLayout::ContainsChildLayout(BOOL fRaw)
{
    DWORD_PTR dw;
    CLayout * pLayout = GetFirstLayout(&dw, FALSE, fRaw);
    ClearLayoutIterator(dw, fRaw);
    return pLayout ? TRUE : FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     Draw
//
//  Synopsis:   Paint the background
//
//----------------------------------------------------------------------------

void
CTableRowLayout::Draw(CFormDrawInfo *pDI, CDispNode *)
{
    AssertSz(FALSE, "Table row doesn't draw itself");
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableRowLayout::Notify     
//
//  Synopsis:   Notification handler.
//
//----------------------------------------------------------------------------

void
CTableRowLayout::Notify(CNotification * pnf)
{
    Assert(!pnf->IsReceived(_snLast));

    if (   !IsInvalidationNotification(pnf)
            ||  ElementOwner() != pnf->Element()
            ||  GetElementDispNode())
    {
        super::Notify(pnf);
    }

#if DBG==1
    // Update _snLast unless this is a self-only notification. Self-only
    // notification are an anachronism and delivered immediately, thus
    // breaking the usual order of notifications.
    if (!pnf->SendToSelfOnly() && pnf->SerialNumber() != (DWORD)-1)
    {
        _snLast = pnf->SerialNumber();
    }
#endif
}

//
// virtual: helper function to calculate absolutely positioned child layout
//

void        
CTableRowLayout::CalcAbsolutePosChild(CCalcInfo *pci, CLayout *pLayout)
{
    CTable *pTable = Table();
    CTableLayout *pTableLayout = pTable? pTable->Layout() : NULL;

    Assert (pLayout->Tag() == ETAG_TD || pLayout->Tag() == ETAG_TH);

    if (pTableLayout)
    {
        CTableCalcInfo tci(pTable, pTableLayout);
        pTableLayout->CalcAbsolutePosCell(&tci, (CTableCell *)pLayout->ElementOwner());
    }

}

//+----------------------------------------------------------------------------
//
//  Member:     RegionFromElement
//
//  Synopsis:   Return the bounding rectangle for an element, if the element is
//              this instance's owner. The RECT returned is in client coordinates.
//
//  Arguments:  pElement - pointer to the element
//              CDataAry<RECT> *  - rectangle array to contain
//              dwflags - flags define the type of changes required
//              (CLEARFORMATS) etc.
//
//-----------------------------------------------------------------------------
void
CTableRowLayout::RegionFromElement(
    CElement *          pElement,
    CDataAry<RECT> *    paryRects,
    RECT *              prcBound,
    DWORD               dwFlags)
{
    if (TableLayout())
        TableLayout()->RegionFromElement(pElement, paryRects, prcBound, dwFlags);
}


//+---------------------------------------------------------------------------
//
//  Member:     ShowSelected     
//
//  Synopsis:   Show the selected range.
//
//----------------------------------------------------------------------------

VOID
CTableRowLayout::ShowSelected(
            CTreePos* ptpStart, 
            CTreePos* ptpEnd, 
            BOOL fSelected, 
            BOOL fLayoutCompletelyEnclosed,
            BOOL fFireOM)
            
{
    if (TableLayout())
        TableLayout()->ShowSelected(ptpStart, ptpEnd, fSelected, fLayoutCompletelyEnclosed, fFireOM );
}


//+---------------------------------------------------------------------------
//
//  Member:     AddCell
//
//  Synopsis:   Add a cell to the row
//
//----------------------------------------------------------------------------

HRESULT
CTableRowLayout::AddCell(CTableCell * pCell)
{
    HRESULT         hr = S_OK;
    CTableLayout  * pTableLayout = TableLayout();
    int             cCols = _aryCells.Size();
    CTableRow     * pRow = TableRow();

    Assert(pCell);

    CTableCellLayout *pCellLayout = pCell->Layout();
    Assert (pCellLayout);

    int             cColSpan = pCell->ColSpan();
    int             cRowSpan = 1;
    int             iAt = 0;
    CTableCell *    pCellOverlap = NULL;
    BOOL            fAbsolutePositionedRow = FALSE;
    BOOL            fDisplayNoneRow = FALSE;
    BOOL            fDisplayNoneCell = FALSE;

    // fill first empty cell

    while (iAt < cCols && !IsEmpty(_aryCells[iAt]))
        iAt++;

    // tell cell where it is
    
    pCellLayout->_iCol = iAt;

    CTreeNode *pNode = pCell->GetFirstBranch();
    const CFancyFormat * pFF = pNode->GetFancyFormat();
    const CParaFormat * pPF = pNode->GetParaFormat();

    if (!pTableLayout)
        goto Cleanup;

    // BUGBUG: we should set this flag based on _dp._fContainsHorzPercentAttr, but it doesn't work correctly
    pTableLayout->_fHavePercentageInset |= (   (pFF->_cuvPaddingLeft.GetUnitType()  == CUnitValue::UNIT_PERCENT)
                                            || (pFF->_cuvPaddingRight.GetUnitType() == CUnitValue::UNIT_PERCENT)
                                            || (pPF->_cuvTextIndent.GetUnitType()   == CUnitValue::UNIT_PERCENT));

    cRowSpan = pCell->GetAArowSpan();
    Assert (cRowSpan >= 1);
    if (cRowSpan > 1)
    {
        pRow->_fHaveRowSpanCells = TRUE;
        const CFancyFormat * pFFRow = pRow->GetFirstBranch()->GetFancyFormat();
        if (pFFRow->_bPositionType == stylePositionabsolute)
            fAbsolutePositionedRow = TRUE;
        if (pFFRow->_bDisplay == styleDisplayNone)
            fDisplayNoneRow = TRUE;
    }

    fDisplayNoneCell = pFF->_bDisplay == styleDisplayNone;
    
    if (   fDisplayNoneRow 
        || fAbsolutePositionedRow 
        || fDisplayNoneCell)
    {
        hr = AddDisplayNoneCell(pCell);
        if (hr)
            goto Cleanup;
        goto Done;
    }

    if (pFF->_bPositionType == stylePositionabsolute)
    {
        hr = AddDisplayNoneCell(pCell);
        if (hr)
            goto Cleanup;
        hr = pTableLayout->AddAbsolutePositionCell(pCell);
        if (hr)
            goto Cleanup;
        goto Done;
    }

    // expand row

    if (cCols < iAt + cColSpan)
    {
        hr = EnsureCells(iAt + cColSpan);
        if (hr)
            goto Cleanup;

        hr = pTableLayout->EnsureCols(iAt + cColSpan);
        if (hr)
            goto Cleanup;

        _aryCells.SetSize(iAt + cColSpan);
    }

    pTableLayout->SetLastNonVirtualCol(iAt);

    Assert(IsEmpty(_aryCells[iAt]));
    _aryCells[iAt] = pCell;

    pCellLayout->_fNotInAryCells = FALSE;
    
    if (cRowSpan != 1)
    {
        hr = pTableLayout->EnsureRowSpanVector(iAt + cColSpan);
        if (hr)
            goto Cleanup;
        pTableLayout->AddRowSpanCell(iAt, cRowSpan);

        // 71720: Turn off _fAllRowsSameShape optimization when we run into rowspanned cells.
        // Note we are not using RowSpan() because we haven't seen the next row yet to see if it is in the same section.
        pTableLayout->_fAllRowsSameShape = FALSE;
    }
    else
    {
        // cache height attribute in row
        const   CHeightUnitValue * puvHeight;

        puvHeight = (CHeightUnitValue *)&pFF->_cuvHeight;
        if (puvHeight->IsSpecified())
        {
            pRow->_fAdjustHeight = TRUE;
        }
    }

    pTableLayout->_cTotalColSpan += cColSpan;   // total number of col spans for this row

    cColSpan--;

    while (cColSpan--)
    {
        ++iAt;
        if (!IsEmpty(_aryCells[iAt]))
        {
            pCellOverlap = Cell(_aryCells[iAt]);
            if (pCellOverlap->Layout()->_iCol == iAt)
            {
                _aryCells[iAt] = MarkSpannedAndOverlapped(pCellOverlap);    // overlapped cell is always spanned
            }
        }
        else
        {
            _aryCells[iAt] = MarkSpanned(pCell);
        }
    }

Done:
    // increment the number of real cells
    _cRealCells++;

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     AddDisplayNoneCell
//
//  Synopsis:   Add a display-none cell to the row
//
//----------------------------------------------------------------------------

HRESULT
CTableRowLayout::AddDisplayNoneCell(CTableCell * pCell)
{
    HRESULT hr;
    CDispNode * pDispNode = NULL;

    Assert (pCell);

    CTableCellLayout *pCellLayout = pCell->Layout();
    Assert (pCellLayout);


    if (!_pDisplayNoneCells)
    {
        _pDisplayNoneCells = new  (Mt(CTableRowLayout_pDisplayNoneCells_pv)) CPtrAry<CTableCell *> (Mt(CTableRowLayout_pDisplayNoneCells_pv));
        if (!_pDisplayNoneCells)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    hr = _pDisplayNoneCells->Append(pCell);

    pDispNode = pCellLayout->GetElementDispNode();
    if (pDispNode)
    {
        pDispNode->ExtractFromTree();
    }

    pCellLayout->_fNotInAryCells = TRUE;
    
Cleanup:
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     EnsureCells
//
//  Synopsis:   Make sure there are at least cCells number of slots in the row
//
//----------------------------------------------------------------------------

HRESULT
CTableRowLayout::EnsureCells(int cCells)
{
    int c = _aryCells.Size();
    HRESULT hr = _aryCells.EnsureSize(cCells);
    if (hr)
        goto Cleanup;

    Assert(c <= cCells);
    _aryCells.SetSize(cCells);
    while (cCells-- > c)
    {
        _aryCells[cCells] = ::MarkEmpty();
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     ClearRowLayoutCache
//
//  Synopsis:   Empty the array of cells.
//
//----------------------------------------------------------------------------

void
CTableRowLayout::ClearRowLayoutCache()
{
    _aryCells.DeleteAll();
    _cRealCells = 0;
    if (_pDisplayNoneCells)
    {
        _pDisplayNoneCells->DeleteAll();
        delete _pDisplayNoneCells;
        _pDisplayNoneCells = NULL;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     AdjustHeight
//
//  Synopsis:   adjust height of the row for specified height of the cell
//
//-------------------------------------------------------------------------

void 
CTableRowLayout::AdjustHeight(CTreeNode *pNodeCell, CCalcInfo *pci, CTable *pTable)
{
    // cache height attribute in row
    const   CHeightUnitValue * puvHeight;

    puvHeight = (CHeightUnitValue *)&pNodeCell->GetFancyFormat()->_cuvHeight;
    if (puvHeight->IsSpecified())
    {
        // set row unit height if not set or smaller then the cell height
        if (!IsHeightSpecified())
        {
            goto Adjustment;
        }
        else if (puvHeight->IsSpecifiedInPercent())
        {
            // set if smaller
            if (IsHeightSpecifiedInPercent())
            {
                if (GetHeightUnitValue() < puvHeight->GetUnitValue())
                {
                    goto Adjustment;
                }
            }
            else
            {
                // percent has precedence over normal height
                goto Adjustment;
            }
        }
        else if (!IsHeightSpecifiedInPercent())
        {
            // set if smaller
            if (GetPixelHeight(pci) < puvHeight->GetPixelHeight(pci, pTable))
            {
                goto Adjustment;
            }
        }
    }
    return;

Adjustment:
    _uvHeight = *puvHeight;
    return;
}


void 
CTableRowLayout::CacheRowHeight(const CFancyFormat *pff) 
{
    if (!pff->_cuvHeight.IsNullOrEnum() || IsHeightSpecified())
    {
        _uvHeight = pff->_cuvHeight; // when height is specified, cache the value here
    }
}

//+------------------------------------------------------------------------
//
//  Member:     GetPositionInFlow
//
//  Synopsis:   Return the in-flow position of a site
//
//-------------------------------------------------------------------------

void
CTableRowLayout::GetPositionInFlow(CElement * pElement, CPoint * ppt)
{
    Assert(GetFirstBranch()->GetUpdatedParentLayoutElement() == Table());

    if (TableLayout())
        TableLayout()->GetPositionInFlow(pElement, ppt);
}
