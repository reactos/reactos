//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltcalc.cxx
//
//  Contents:   CTableLayout calculating layout methods.
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

#ifndef X_LSM_HXX
#define X_LSM_HXX
#include "lsm.hxx"
#endif

//#define TABLE_PERF 1
#ifdef TABLE_PERF
#include "icapexp.h"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

MtDefine(CTableLayoutCalculateMinMax_aryReducedSpannedCells_pv, Locals, "CTableLayout::CalculateMinMax aryReducedSpannedCells::_pv")
MtDefine(CalculateLayout, PerfPigs, "CTableLayout::CalculateLayout")

DeclareTag(tagTableRecalc,        "TableRecalc",  "Allow incremental recalc")
DeclareTag(tagTableChunk,         "TableChunk",   "Trace table chunking behavior")
DeclareTag(tagTableCalc,          "TableCalc",    "Trace Table/Cell CalcSize calls")
DeclareTag(tagTableDump,          "TableDump",    "Dump table sizes after CalcSize")
DeclareTag(tagTableSize,          "TableSize",    "Check min cell size two ways")
DeclareTag(tagTableMinAssert,     "TableMinAssert", "Assert if SIZEMODE_MINWIDTH is used")
DeclareTag(tagTableCellSizeCheck, "TableCellCheck",  "Check table cell size against min size")

ExternTag(tagCalcSize);

PerfTag(tagTableMinMax, "TableMinMax", "CTable::CalculateMinMax")
PerfTag(tagTableLayout, "TableLayout", "CTable::CalculateLayout")
PerfTag(tagTableColumn, "TableColumn", "CTable::CalculateColumns")
PerfTag(tagTableRow,    "TableRow",    "CTable::CalculateRow")
PerfTag(tagTableSet,    "TableCell",   "CTable::SetCellPositions")

// Wrappers to make is easier to profile table cell size calcs.
void CalculateCellMinMax (CTableCellLayout *pCellLayout,
                          CTableCalcInfo * ptci,
                          SIZE *psize)
{
    pCellLayout->CalcSize(ptci, psize);
}

void CalculateCellMin (CTableCellLayout *pCellLayout,
                       CTableCalcInfo * ptci,
                       SIZE *psize)
{
    pCellLayout->_fMinMaxValid = FALSE;
    ptci->_smMode = SIZEMODE_MINWIDTH;
    pCellLayout->CalcSize(ptci, psize);
    ptci->_smMode = SIZEMODE_MMWIDTH;
    pCellLayout->_fMinMaxValid = TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CalculateMinMax
//
//  Synopsis:   Calculate min/max width of table
//
//  Return:     sizeMin.cx, sizeMax,cx, if they are < 0 it means CalcMinMax
//              have failed during incremental recalc to load history
//
//--------------------------------------------------------------------------

void
CTableLayout::CalculateMinMax(CTableCalcInfo * ptci)
{
    SIZEMODE        smMode = ptci->_smMode; // save
    int             cR, cC;
    CTableColCalc * pColCalc;
    CTableColCalc * pSizedColCalc;
    int             cCols = GetCols();
    CTableRow **    ppRow;
    CTableRowLayout * pRowLayout;
    int             cRows = GetRows();
    CTableCell **   ppCell;
    CTableCell *    pCell;
    CTableCellLayout * pCellLayout;
    CTableCaption **ppCaption;
    int             cColSpan, cRowSpan;
    SIZE            size;
    long            xTablePadding = 0;
    int             cSpanned = 0;   // number of cells with colSpan > 1
    long            xMin=0, xMax=0;
    long            cxWidth, dxRemainder;
    int             cUnsizedCols, cSizedColSpan;
    BOOL            fMinMaxCell;
    const CWidthUnitValue *  puvWidth=NULL;
    BOOL            fTableWidthSpecifiedInPercent;
    long            xTableWidth = GetSpecifiedPixelWidth(ptci, &fTableWidthSpecifiedInPercent);
    CTableColCalc * pColLastNonVirtual = NULL;  // last non virtual column
    int             cReducedSpannedCells;
    CTableColCalc * pColCalcSpanned;
    int             i, iCS;
    CPtrAry<CTableCell *>   aryReducedSpannedCells(Mt(CTableLayoutCalculateMinMax_aryReducedSpannedCells_pv));
    int             iPixelWidth;
    CTable        * pTable = Table();
    CTreeNode     * pNode;
    BOOL            fRowDisplayNone;
    BOOL            fAlwaysMinMaxCells = _fAlwaysMinMaxCells;
    
    BOOL            fCookUpEmbeddedTableWidthForNetscapeCompatibility = (xTableWidth == 0 &&
                        !fTableWidthSpecifiedInPercent && !pTable->IsAligned());
    if (fCookUpEmbeddedTableWidthForNetscapeCompatibility)
    {
        CLayout * pParentLayout = GetUpdatedParentLayout();
        Assert(pParentLayout);

        switch (pParentLayout->Tag())
        {
        case ETAG_TD:
        case ETAG_TH:
        case ETAG_CAPTION:
            pCellLayout = DYNCAST(CTableCellLayout, pParentLayout);

            // Only apply Nav compatibility width when there is nothing
            // else aligned to this table.
            if (!pCellLayout->_fAlignedLayouts)
                xTableWidth = pCellLayout->GetSpecifiedPixelWidth(ptci);

        default:
            fCookUpEmbeddedTableWidthForNetscapeCompatibility = (xTableWidth != 0);
        }
    }

#if 0
    int xRowMin, xRowMax;
#endif

    PerfLog(tagTableMinMax, this, "+CalculateMinMax");

    ptci->_smMode = SIZEMODE_MMWIDTH;

    // Calc and cache border and other values
    CalculateBorderAndSpacing(ptci);

    _sizeMin.cx = 0;
    _sizeMax.cx = 0;

    size.cx = 0;
    size.cy = 0;

    // ensure columns

    if (_aryColCalcs.Size() < cCols)
    {
        _aryColCalcs.EnsureSize(cCols);
        _aryColCalcs.SetSize(cCols);
    }

    // reset column values
    for (i = 0, pColCalc = _aryColCalcs; i < cCols; i++, pColCalc++)
    {
        pColCalc->Clear();
        CTableCol *pCol = GetCol(i);
        if (pCol && pCol->IsDisplayNone())
        {
            pColCalc->_fDisplayNone = TRUE;
        }
    }

    _cNonVirtualCols = 0;
    _cSizedCols  = min(cCols, pTable->GetAAcols());
    cUnsizedCols = cCols - _cSizedCols;
    _fCols = !cUnsizedCols;
    fAlwaysMinMaxCells |=  ((ptci->_grfLayout & LAYOUT_FORCE) && _cSizedCols);

    if (IsHistoryAvailable())
    {
        LoadHistory(pTable->_pStreamHistory, ptci);

        ClearInterface(&pTable->_pStreamHistory);

        if (_fUsingHistory)
        {
            if (cCols)
                pColLastNonVirtual = &_aryColCalcs[cCols - 1];

            goto endMinMax;
        }
        else
        {
            // load history have failed (_fUsingHistory == FALSE)
            if (_fIncrementalRecalc)
            {
                // if we are in the middle of the incremental recalc, stop it right here
                _fIncrementalRecalc = FALSE;
                if (!_fCompleted)
                {
                    // if we are not completed Ensure full MinMax path 
                    // (table will request resize at the compleetion, so we don't need to do it here).
                    _sizeMin.cx = -1;
                    _sizeMax.cx = -1;
                    goto EmergencyExit;
                }   // if we are completed then do full MinMax path now
            }
        }
    }
    else
    {
        _fUsingHistory = FALSE;
    }

    if (IsFixed())
    {
        // Set Columns Width
        CTableCol   *pCol;
        int         iSpecMinSum  = 0;
        int         iSpecedCols  = 0;
        int         iDisplayNoneCols  = 0;
        int         iBigNumber   = INT_MAX/2;
        int         iColumnWidth = 1;
        int         iColSpan;
        CUnitValue  uvWidth;
        int         iFirstRow = GetFirstRow();
        BOOL        fSpecifiedColWidth;
        pRowLayout = _aryRows.Size()? _aryRows[iFirstRow]->Layout(): NULL;
        int         ip;
        BOOL        fCellSpecifiedWidth;
        int         iSpan;

        if (pRowLayout && (pRowLayout->GetCells() < cCols))
        {
            pRowLayout->EnsureCells(cCols);
        }

        // go through specified columns and set the colCalcs accordingly
        for (int i = 0; i < _aryColCalcs.Size(); i++)
        {
            fCellSpecifiedWidth = FALSE;

            pCol = i < _aryCols.Size()? _aryCols[i] : NULL;
            if (pCol && pCol->IsDisplayNone())
            {
                iDisplayNoneCols++;
                continue;
            }
            if (pCol && !pCol->IsDisplayNone())
            {
                Assert(pCol->_iCol  == i);  // sanity check
                iColSpan = pCol->Cols();
                Assert (iColSpan >= 1);
                pNode = pCol->GetFirstBranch();
                uvWidth = pNode->GetCascadedwidth();
                puvWidth = (CWidthUnitValue *)&uvWidth;
                fSpecifiedColWidth = puvWidth->IsSpecified();
            }
            else
            {
                iColSpan = 1;
                fSpecifiedColWidth = FALSE;
            }

            if (   !fSpecifiedColWidth 
                && ((pCol && iColSpan == 1) || !pCol)
                && pRowLayout 
                && IsReal(pRowLayout->_aryCells[i]))
            {
                pCell = Cell(pRowLayout->_aryCells[i]);
                Assert (pCell);
                pNode = pCell->GetFirstBranch();
                puvWidth = (CWidthUnitValue *)&pNode->GetFancyFormat()->_cuvWidth;
                fSpecifiedColWidth = puvWidth->IsSpecified();
                if (fSpecifiedColWidth)
                {
                    iColSpan = pCell->ColSpan();
                    fCellSpecifiedWidth = TRUE;
                }
            }
            if (fSpecifiedColWidth)
            {
                if (puvWidth->IsSpecifiedInPixel())
                {
                    iPixelWidth = puvWidth->GetPixelWidth(ptci, pTable);
                }
                else
                {
                    ip = puvWidth->GetPercent();
                    iPixelWidth = _sizeParent.cx*ip/100;
                }
                iColumnWidth = iPixelWidth;

                if (fCellSpecifiedWidth)
                {
                    iColumnWidth /= iColSpan;
                    iSpan = iColSpan;
                }
                else
                {
                    iSpan = 1;
                }

                for (int j = 0 ; j < iColSpan; j++)
                {
                    pColCalc = &_aryColCalcs[i + j];
                    // if it is a cell specified width with the colSpan > 1 and it is the last column, then
                    if (fCellSpecifiedWidth && iColSpan > 1 && j == iColSpan - 1)
                    {
                        // this is the size of the last column
                        iColumnWidth = iPixelWidth - (iColSpan - 1)*iColumnWidth;
                    }
                    pColCalc->AdjustForCol(puvWidth, iColumnWidth, ptci, iSpan);
                    iSpecedCols++;
                }
                iSpecMinSum += iPixelWidth;

            }
            i += iColSpan - 1;
        }

        // If any columns didn't specify a width, set their min to 1 (almost zero) and
        // their max to infinity (sums maximally add up to INT_MAX/2).  These values
        // make CTableLayout::CalculateColumns distribute any available width equally
        // among the unspecified columns.
        if (cCols - iSpecedCols - iDisplayNoneCols)
        {
            Assert (iSpecedCols < cCols);

            iColumnWidth = max(1, (iBigNumber - iSpecMinSum)/(cCols - iSpecedCols));

            for (cC = cCols, pColCalc = _aryColCalcs;
                cC > 0;
                cC--, pColCalc++)
            {
                if (pColCalc->_fDisplayNone)
                    continue;
                if (!pColCalc->IsWidthSpecified())
                {
                    pColCalc->_xMin = 1;
                    pColCalc->_xMax = iColumnWidth;
                }
            }
        }

        if (cCols)
            pColLastNonVirtual = &_aryColCalcs[cCols - 1];

        goto endMinMax;
    }

    if (_cNestedLevel > SECURE_NESTED_LEVEL)
    {
        if (cCols)
            pColLastNonVirtual = &_aryColCalcs[cCols - 1];
        goto endMinMax;
    }

    //
    // Determine the number of fixed size columns (if any)
    // (If the COLS attribute is specified, assume the user intended
    //  for it to work and disregard any differences between the
    //  the number of columns specified through it and those
    //  actually in the table)
    //

    // collect min/max information for all the cells and columns
    // (it is safe to directly walk the row array since the order in which
    //  header/body/footer rows are encountered makes no difference)

    _fZeroWidth  = TRUE;

    for (cR = cRows, ppRow = _aryRows;
        cR > 0;
        cR--, ppRow++)
    {
        pRowLayout = (*ppRow)->Layout();

        // Ensure the row contains an entry for all columns
        // (It will not if subsequent rows have more cells)
        if (pRowLayout->GetCells() < cCols)
        {
            pRowLayout->EnsureCells(cCols);
        }

#if 0
        // reset row values
        xRowMin = xRowMax = 0;
#endif
        pRowLayout->_uvHeight.SetNull();

        Assert(pRowLayout->GetCells() == cCols);
        ppCell = pRowLayout->_aryCells;

        fRowDisplayNone = pRowLayout->GetFirstBranch()->IsDisplayNone();

        for (cC = cCols, pColCalc = _aryColCalcs ;
             cC > 0;
             cC -= cColSpan, ppCell += cColSpan, pColCalc += cColSpan)
        {
            if (pColCalc->IsDisplayNone())
            {
                cColSpan = 1;
                continue;
            }
            pCell = Cell(*ppCell);

            if (IsReal(*ppCell))
            {
                pCellLayout = pCell->Layout();
                if (pColCalc > pColLastNonVirtual)
                {
                    pColLastNonVirtual = pColCalc;
                }

                cColSpan = pCell->ColSpan();

                // if row style is display==none, don't calculate min-max for it's cells
                if (fRowDisplayNone)
                    continue;

                cRowSpan = pCell->RowSpan();
                pNode = pCell->GetFirstBranch();
                iPixelWidth = pCellLayout->GetSpecifiedPixelWidth(ptci);
                puvWidth = (CWidthUnitValue *)&pNode->GetFancyFormat()->_cuvWidth;

                if (cR == cRows)
                {
                    pColCalc->_fWidthInFirstRow = puvWidth->IsSpecified();

                    for (iCS = 1, pColCalcSpanned = pColCalc + 1;
                         iCS < cColSpan;
                         iCS++, pColCalcSpanned++)
                    {
                        pColCalcSpanned->_fWidthInFirstRow = pColCalc->_fWidthInFirstRow;
                    }
                }

                fMinMaxCell        = (cC <= cUnsizedCols           ||
                                      !pColCalc->_fWidthInFirstRow ||
                                      (cR == cRows && puvWidth->IsSpecifiedInPercent()));
                _fCols = _fCols && !fMinMaxCell;

                //
                // For fixed size columns, use the supplied width for their min/max
                // (But ensure it is no less than that needed to display borders)
                //

                if (!fMinMaxCell)
                {
                    if (cR == cRows)
                    {
                        xMin =
                        xMax = puvWidth->GetPixelWidth(ptci, pTable, 0) + pCellLayout->GetBorderAndPaddingWidth(ptci);
                    }
                    else
                    {
                        xMin =
                        xMax = pColCalc->_xMax;
                    }
                }

                //
                // For non-fixed size columns, determine their min/max values
                //

                if (fMinMaxCell || fAlwaysMinMaxCells)
                {
                    WHEN_DBG ( BOOL fNeededMinWidth = FALSE; )

                    //
                    // Attempt to get min/max in a single pass
                    //

                    pCellLayout->_fDelayCalc = FALSE;
                    if (   fAlwaysMinMaxCells 
                        || (   (_fHavePercentageInset || (_fForceMinMaxOnResize && pCellLayout->_fForceMinMaxOnResize))  
                            && pCellLayout->_fMinMaxValid) )
                    {
                        pCellLayout->_xMax = pCellLayout->_xMin = -1;
                        pCellLayout->_fMinMaxValid = FALSE;
                    }
                    CalculateCellMinMax(pCellLayout, ptci, &size);
                    if (_fZeroWidth && !pCellLayout->NoContent())
                        _fZeroWidth = FALSE;

                    // if cell has a space followed by <br>, if we would have used history the text would create an extra 
                    // line for that <br>, since we would give the text exact width and it breaks after the space and then
                    // creates another line when it encounters <br> 
                    // ( it doesn't do look aside for <br> to catch this situation). bug #49599
                    if (pCellLayout->_dp.GetNavHackPossible())
                    {
                        _fDontSaveHistory = TRUE;
                        // _fForceMinMaxOnResize = TRUE;  // set it only during NATURAL calc, since we don't want to do minmax more then once.
                        pCellLayout->_fForceMinMaxOnResize = TRUE;  
                    }
                    else
                    {
                        pCellLayout->_fForceMinMaxOnResize = FALSE;
                    }


                    //
                    // If minimum width could not be reliably calculated, request it again
                    //

                    if (    pCellLayout->_fAlignedLayouts 
#if DBG == 1
                        ||  IsTagEnabled(tagTableSize)
#endif
                       )
                    {
                        SIZE    sizeMin;

                        WHEN_DBG ( fNeededMinWidth = TRUE; )
                        CalculateCellMin(pCellLayout, ptci, &sizeMin);
#if DBG == 1
                        Assert(!IsTagEnabled(tagTableSize) || sizeMin.cx == size.cy);
                        Assert(!IsTagEnabled(tagTableMinAssert));
#endif

                        size.cy = max(sizeMin.cx, size.cy);
                        pCellLayout->_xMin = size.cy;   // we need to set correct value for _xMin, so next time FlowLayout returns correct min value
                    }

                    //
                    // For normal min/max cases, use the returned value
                    //

                    if (fMinMaxCell)
                    {
                        xMax = size.cx;
                        xMin = size.cy;
                        Assert (fNeededMinWidth || xMax >= xMin);

//                        if (pCellLayout->IsWhiteSpacesOnly())
//                        {
//                            xMin = 0;
//                        }

                        // If a user set value exists, set the cell's maximum value
                        if (cC <= cUnsizedCols && puvWidth->IsSpecified() && puvWidth->IsSpecifiedInPixel() && iPixelWidth)
                        {
                            xMax = iPixelWidth;
                            // Ensure supplied width is not less than that of the minimum content width
                        }
                        if (xMax < xMin)
                        {
                            xMax = xMin;
                        }
                    }

                    //
                    // For fixed size cells, increase the min/max only
                    // if the calculated min is greater
                    //

                    else
                    {
                        Assert(cC > cUnsizedCols);
                        xMin =
                        xMax = max((long)xMax, size.cy);
                    }
                }


                //
                // For non-spanned cells, move the min/max (and possibly the
                // specified width) into the column structure
                //

                if (cColSpan == 1)
                {
                    pColCalc->AdjustForCell(this, iPixelWidth, puvWidth,
                                        (cC <= cUnsizedCols || !pColCalc->_fWidthInFirstRow),
                                        cR == cRows, ptci, xMin, xMax);
                }

                //
                // For spanned cells, distribute the width over the affected cells
                //

                else
                {

                    // if the spanned cell is exactly at the end of the table, then it is a potential
                    // case gor ignoring colSpans
                    if (cCols - cC == _iLastNonVirtualCol)
                    {
                        aryReducedSpannedCells.Append(pCell);
                    }

                    // Netscape compatibility (garantee for 1 + _xCellSpacing pixels per column)
                    int iMakeMeLikeNetscape = (cColSpan - 1)*(_xCellSpacing + 1);
                    if (iMakeMeLikeNetscape > pCellLayout->_xMin)
                    {
                        pCellLayout->_xMax += iMakeMeLikeNetscape - pCellLayout->_xMin;
                        pCellLayout->_xMin = iMakeMeLikeNetscape;
                    }

                    //
                    // For non-fixed size spanned cells, simply note that the cell spans
                    // (The distribution cannot take place until after all cells in the
                    //  all the rows are have their min/max values determined)
                    //

                    if (    (   cC <= cUnsizedCols
                            || (    fMinMaxCell
                                &&  cR == cRows))
                        ||  (   (fAlwaysMinMaxCells)
                            &&  cR != cRows))
                    {
                        cSpanned++;
                        Assert (!pCellLayout->GetFirstBranch()->IsDisplayNone());
                        Assert(pCellLayout->_fMinMaxValid);
                        // NETSCAPE: uses special algorithm for calculating min/max of virtual columns
                        for (iCS = 0, pColCalcSpanned = pColCalc; iCS < cColSpan; iCS++, pColCalcSpanned++)
                        {
                            pColCalcSpanned->_cVirtualSpan++;
                        }
                    }

                    //
                    // For fixed size cells, distribute the space immediately
                    // (Fixed size cells whose widths are either unspecified or are
                    //  a percentage will have their widths set during CalculateColumns)
                    //

                    else if (!fMinMaxCell && cR == cRows)
                    {
                        Assert(cC > cUnsizedCols);

                        cxWidth       = xMax;
                        cSizedColSpan = min(cColSpan, _cSizedCols - (cCols - cC));

                        //
                        // Divide the user width over the affected columns
                        //

                        dxRemainder  = cxWidth - ((cSizedColSpan - 1) * _xCellSpacing);
                        cxWidth      = dxRemainder / cSizedColSpan;
                        dxRemainder -= cxWidth * cSizedColSpan;

                        //
                        // Set the min/max and width of the affected columns
                        //

                        Assert(cSizedColSpan <= (_aryColCalcs.Size() - (pColCalc - (CTableColCalc *)&_aryColCalcs[0])));
                        pSizedColCalc = pColCalc;

                        do
                        {

                            pSizedColCalc->Set(cxWidth + (dxRemainder > 0
                                                            ? 1
                                                            : 0));
                            if (pSizedColCalc > pColLastNonVirtual)
                            {
                                pColLastNonVirtual = pSizedColCalc;
                            }
                            pSizedColCalc++;
                            cSizedColSpan--;
                            dxRemainder--;

                        } while (cSizedColSpan);
                    }
                }

                if (cRowSpan == 1 && (*ppRow)->_fAdjustHeight)
                {
                    pRowLayout->AdjustHeight(pNode, ptci, pTable);
                }
            }
            else
            {
                cColSpan = 1;
            }
            Assert(pColCalc->_xMin <= pColCalc->_xMax);
        }
    }

    //
    // If cells were spanned, check them again now
    //

    if (cSpanned)
    {
        cReducedSpannedCells = 0;
        for (int i=0; i < aryReducedSpannedCells.Size(); i++)
        {
            pCell = aryReducedSpannedCells[i];
            pCellLayout = pCell->Layout();

            pColCalc = _aryColCalcs;
            pColCalc += pCellLayout->_iCol;
            cColSpan = pCell->ColSpan();
            BOOL fIgnoreSpan = TRUE;
            if (pColCalc->_cVirtualSpan != 1)
            {
                CTableColCalc *pColCalcSpannedPrev = pColCalc;
                pColCalcSpanned = pColCalc + 1;
                for (iCS = 1; iCS < cColSpan; iCS++, pColCalcSpanned++)
                {
                    Assert (pColCalcSpanned->_cVirtualSpan <= pColCalcSpannedPrev->_cVirtualSpan);
                    if (pColCalcSpanned->_cVirtualSpan !=
                        pColCalcSpannedPrev->_cVirtualSpan)
                    {
                        fIgnoreSpan = FALSE;
                        pColLastNonVirtual = pColCalcSpanned;
                    }
                    pColCalcSpannedPrev = pColCalcSpanned;
                }
            } // else we can ignore the SPAN
            if (fIgnoreSpan)
            {
                iPixelWidth = pCellLayout->GetSpecifiedPixelWidth(ptci);
                puvWidth = (CWidthUnitValue *)&pCell->GetFirstBranch()->GetFancyFormat()->_cuvWidth;
                pColCalc->AdjustForCell(this, iPixelWidth, puvWidth, TRUE, TRUE, ptci, pCellLayout->_xMin, pCellLayout->_xMax);
                cReducedSpannedCells++;
            }
        }
        if (cSpanned - cReducedSpannedCells)
        {
            AdjustForColSpan(ptci, pColLastNonVirtual);
        }
    }

    if (   (_fCompleted && _fCalcedOnce)    // if min max path was more then once,
        || ptci->_fDontSaveHistory)         // or there are still not loaded images
    {
        _fDontSaveHistory = TRUE;           // then don't save the history.
    }

endMinMax:

    if (_pAbsolutePositionCells)
    {
        CTableCellLayout *pCellLayout;
        CTableCell      **ppCell;
        int               cCells;
        for (cCells = _pAbsolutePositionCells->Size(), ppCell = *_pAbsolutePositionCells ;  cCells > 0; cCells--, ppCell++)
        {
            pCellLayout = (*ppCell)->Layout();
            CalculateCellMinMax(pCellLayout, ptci, &size);
        }
    }

    // calculate min/max table width and height

    xMin = xMax = 0;

    // sum up columns and check width

    if (cCols)
    {
        for (pColCalc = _aryColCalcs; pColCalc <= pColLastNonVirtual; pColCalc++)
        {
            xMin += pColCalc->_xMin;
            xMax += pColCalc->_xMax;
            if (!_fUsingHistory && !pColCalc->_fVirtualSpan && !pColCalc->IsDisplayNone())
            {
                _cNonVirtualCols++;
            }
        }
    }

    if (_sizeMin.cx < xMin)
    {
        _sizeMin.cx = xMin;
    }
    if (_sizeMax.cx < xMax)
    {
        _sizeMax.cx = xMax;
    }

    // add border space and padding

    if (_sizeMin.cx != 0 || _sizeMax.cx != 0)
    {
        // NETSCAPE: doesn't add the border or spacing if the table is empty.

        xTablePadding = _aiBorderWidths[BORDER_RIGHT] + _aiBorderWidths[BORDER_LEFT] + _cNonVirtualCols * _xCellSpacing + _xCellSpacing;
        _sizeMin.cx += xTablePadding;
        _sizeMax.cx += xTablePadding;
    }

    if (fCookUpEmbeddedTableWidthForNetscapeCompatibility)
    {
        if ( _sizeMax.cx < xTableWidth ||
             (_cNonVirtualCols == 1 && !(--pColCalc)->IsWidthSpecifiedInPixel()))
        {
            xTableWidth = 0;    //  DON'T CookUpEmbeddedTableWidthForNetscapeCompatibility
        }
    }

    // check if caption forces bigger width

    // NOTE:   NETSCAPE: does not grow the table width to match that of the caption, yet
    //         we do. If this becomes a problem, we can alter the table code to maintain
    //         a larger RECT which includes both the caption and table while the table
    //         itself is rendered within that RECT to its normal size. I've avoided adding
    //         this for now since it is not trivial. (brendand)

    for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
         cC > 0;
         cC--, ppCaption++)
    {
        CTableCellLayout *pCaptionLayout = (*ppCaption)->Layout();

        // Captions don't always have/need layout (replacing the above call w/ GetUpdatedLayout() doesn't
        // necessarily result in a layout ptr).  Added if() check; bug #75543.
        if ( pCaptionLayout )
        {
            pCaptionLayout->CalcSize(ptci, &size);

            if (_fZeroWidth && !pCaptionLayout->NoContent())
                _fZeroWidth = FALSE;

            if (_sizeMin.cx < size.cy)
            {
                _sizeMin.cx = size.cy;
            }

            // NETSCAPE: Ensure the table is wide enough for the minimum CAPTION width only
            //           (see above comments) (brendand)
            if (_sizeMax.cx < size.cy)
            {
                _sizeMax.cx = size.cy;

            }

            // If the table contains only a CAPTION, then use its maximum width for the table
            if (!GetRows() && _sizeMax.cx < size.cx)
            {
                _sizeMax.cx = size.cx;
            }

            /*
            _sizeMin.cy += size.cy;
            _sizeMax.cy += size.cy;
            */
        }
    }


    // If user specified the width of the table, we want to restrict max to the
    // specified width.
    // NS/IE compatibility, any value <= 0 is treated as <not present>
    if (xTableWidth)
    {
        if (xTableWidth > _sizeMin.cx)
        {
            _sizeMin.cx =
            _sizeMax.cx = xTableWidth;
        }
    }

    Assert(_sizeMin.cx <= _sizeMax.cx);

EmergencyExit:
    ptci->_smMode = smMode; // restore

    PerfLog(tagTableMinMax, this, "-CalculateMinMax");
}


//+-------------------------------------------------------------------------
//
//  Method:     AdjustForColSpan
//
//  Synopsis:
//
//--------------------------------------------------------------------------

void
CTableLayout::AdjustForColSpan(CTableCalcInfo * ptci, CTableColCalc *pColLastNonVirtual)
{
    int             cR, cC;
    int             iCS;
    CTableColCalc * pColCalc=0;
    CTableColCalc * pColCalcLast=0;
    CTableColCalc * pColCalcBase;
    int             cCols = GetCols();
    CTableRow **    ppRow;
    CTableRowLayout * pRowLayout;
    int             cRows = GetRows();
    CTableCell **   ppCell;
    CTableCell *    pCell;
    CTableCellLayout * pCellLayout;
    int             cColSpan, cRowSpan;
    int             xMin, xMax;
    int             cUnsizedCols;
    const CWidthUnitValue *  puvWidth=NULL;
    CTable      *   pTable = ptci->Table();
    BOOL            fAlwaysMinMaxCells =  _fAlwaysMinMaxCells || ((ptci->_grfLayout & LAYOUT_FORCE) && _cSizedCols);

    cUnsizedCols = cCols - _cSizedCols;

    //
    // have to run through again and check column widths for spanned cells
    // if the sum of widths of the spanned columns are smaller then the
    // width of the cell we have to distribute the extra width
    // amongst the columns to make sure the cell will fit
    // (it is safe to directly walk the row array since the order in which
    //  header/body/footer rows are encountered makes no difference)
    //

    for (cR = cRows, ppRow = _aryRows;
        cR > 0;
        cR--, ppRow++)
    {
        pRowLayout = (*ppRow)->Layout();

        if (pRowLayout->IsDisplayNone())
            continue;

        Assert(pRowLayout->GetCells() == cCols);
        ppCell = pRowLayout->_aryCells;

        for (cC = cCols, pColCalcBase = _aryColCalcs;
            cC > 0;
            cC -= cColSpan, ppCell += cColSpan, pColCalcBase += cColSpan)
        {
            if (pColCalcBase->IsDisplayNone())
            {
                cColSpan = 1;
                continue;
            }
            pCell = Cell(*ppCell);

            if (!IsReal(*ppCell))
            {
                cColSpan = 1;
            }
            else
            {
                pCellLayout = pCell->Layout();
                cColSpan = pCell->ColSpan();
                cRowSpan = pCell->RowSpan();

                // if the cell spans across multiple columns
                Assert (pColCalcBase <= pColLastNonVirtual);
                if (cColSpan > 1 &&
                    (fAlwaysMinMaxCells || cC <= cUnsizedCols) &&
                    pColCalcBase < pColLastNonVirtual)
                {
                    //
                    // cell is spanned, get min and max size and distribute it amongst columns
                    //

                    int iWidth = 0;     // Width (user specified) of this cell
                    int iWidthMin = 0;  // Min width of all the columns spanned accross this cell
                    int iWidthMax = 0;  // Max width of all the columns spanned accross this cell

                    int iUser = 0;      // Width of the pixel's (user) specified columns spanned accross this cell
                    int iUserMin = 0;   // Min width of those columns
                    int iUserMax = 0;   // Max width of those columns
                    int cUser = 0;      // Number of spanned columns that specified pixel's width

                    int iPercent = 0;   // %% width of the %% specified columns spanned accross this cell
                    int iPercentMin = 0;// Min width of those columns
                    int iPercentMax = 0;// Max width of those columns
                    int cPercent = 0;   // Number of columns scpecified with %%

                    int iMax = 0;       // Max width of normal columns (no width were scpecifed)
                    int cMax = 0;       // Number of columns that need Max distribution (normal column, no width specified)

                    int iMinMaxDelta = 0; // the delta between iWidthMax and iWidthMin (all the columns)

                    int xAdjust;        // adjustment to the width of this cell to account
                                        // cell spacing
                    int iMinSum;        // calculated actual Min sum of all the columns
                    int xDistribute;    // Width to distribute between normal columns.
                    int iOriginalColMin;
                    int iOriginalColMax;
                    int cRealColSpan = cColSpan;    // colSpan which doesn't include virtual columns
                    int  cVirtualSpan = 0;    // number of virtual columns
                    BOOL fColWidthSpecified;// set if there is a width spec on column
                    BOOL fColWidthSpecifiedInPercent; // set if there is a width spec in %% on column
                    BOOL fDoNormalMax;      // Do normal Max distribution
                    BOOL fDoNormalMin;      // Do normal Min distribution
                    int  iColMinWidthAdjust;// Adjust the coulmn min by
                    int  iColMaxWidthAdjust;// Adjust the coulmn max by
                    int  iCellPercent = 0;  // the specified %% of the spanned cell
                    int  iColPercent = 0;   // the calculated %% of the column
                    int  iColsPercent = 0;  // total sum of %% for all the columns
                    int  iNormalMin = 0;    // delta between the cell's Min and all the columns Min
                    int  iNormalMax = 0;    // delta between the cell's Max and all the columns Max
                    int  iNormalMaxForPercentColumns = 0;
                    int  iNormalMinForPercentColumns = 0;
                    int  xDistributeMax;
                    int  xDistributeMin;
                    int  xPercent;
                    int  iExtraMax = 0;
                    int  iNewPercentMax = 0;
                    int  cNewPercent = 0;

                    Assert(!pCellLayout->GetFirstBranch()->IsDisplayNone());
                    Assert(pCellLayout->_fMinMaxValid);

                    //
                    // sum up percent and user width columns for later distribution
                    //

                    for (iCS = 0; iCS < cColSpan; iCS++)
                    {
                        pColCalc = pColCalcBase + iCS;  // pColCalc is the column that this Cell is spanned accross

                        if (pColCalc->IsDisplayNone())
                        {
                            cRealColSpan--;
                            continue;
                        }

                        if (pColCalc->IsWidthSpecified())
                        {   // if the size of the coulumn is set, then
                            if (pColCalc->IsWidthSpecifiedInPercent())
                            {
                                iPercent += pColCalc->GetPercentWidth();
                                iPercentMin += pColCalc->_xMin;
                                iPercentMax += pColCalc->_xMax;
                                cPercent++;
                            }
                            else
                            {
                                iUser += pColCalc->GetPixelWidth(ptci, pTable);
                                iUserMin += pColCalc->_xMin;
                                iUserMax += pColCalc->_xMax;
                                cUser++;
                            }
                        }

                        if (pColCalc->_xMax)
                        {
                            iWidthMin += pColCalc->_xMin;
                            iWidthMax += pColCalc->_xMax;
                            if (pColCalc->_fVirtualSpan)
                            {
                                // NETSCAPE: we need to account for the virtual columns
                                cVirtualSpan++;
                            }
                        }
                        else
                        {
                            if (pColCalc <= pColLastNonVirtual)
                            {
                                // NETSCAPE: we need to account for the virtual columns
                                cVirtualSpan++;
                                // NETSCAPE: For each virtual column (due to the colSpan)
                                // they give extra 1 + cellSpacing pixels
                                pColCalc->_xMin =
                                pColCalc->_xMax = _xCellSpacing + 1;
                                iWidthMin += pColCalc->_xMin;
                                iWidthMax += pColCalc->_xMax;
                            }
                            else
                            {
                                // Don't count virtual columns at the end of the table
                                // when distributing.
                                cRealColSpan--;
                            }
                        }
                    }
                    iColsPercent = iPercent;
                    iCellPercent = (cRealColSpan == cPercent) ? iColsPercent : 100;

                    // don't take cell spacing into account
                    xAdjust = (cRealColSpan - cVirtualSpan - 1) * _xCellSpacing;
                    if (xAdjust < 0)
                    {
                        xAdjust = 0;
                    }

                    xMax = pCellLayout->_xMax;    // max width of this cell

                    puvWidth = (CWidthUnitValue *)&pCell->GetFirstBranch()->GetFancyFormat()->_cuvWidth;

                    // use user set value if set
                    if (puvWidth->IsSpecified() && puvWidth->IsSpecifiedInPixel())
                    {
                        iWidth = puvWidth->GetPixelWidth(ptci, pTable);
                        if (iWidth < 0)
                        {
                            iWidth = 0;
                        }

                        if(xMax < iWidth)
                        {
                            xMax = iWidth;
                        }
                    }
                    xMin = pCellLayout->_xMin;    // min width of this cell

                    Assert(xMax >= 0);
                    Assert(xMin >= 0);

                    if (xMax < xMin)
                    {
                        xMax = xMin;
                    }

                    cMax = cRealColSpan - cPercent - cUser;
                    iMax = iWidthMax - iPercentMax - iUserMax;

                    xDistribute = xMax;

                    //
                    // Now check if the cell width is specified by the user
                    //
                    if (puvWidth->IsSpecified())
                    {
                        if (puvWidth->IsSpecifiedInPercent())
                        {
                            // if there is percentage over distribute it
                            // amongst the non-percent columns
                            iCellPercent = puvWidth->GetPercent();
                            iPercent = iCellPercent - iColsPercent;
                            if (iPercent < 0)
                            {
                                iPercent = 0;
                            }
                            iUser = 0;
                        }
                        else
                        {
                            // if there is width over the user set widths and the percentage
                            // distribute it amongst the normal columns
                            iUser =  iWidth - MulDivQuick(iWidth, iPercent, iCellPercent) - iUser;
                            if (iUser < 0)
                            {
                                iUser = 0;
                            }
                            iPercent = 0;
                            xDistribute = iWidth;
                        }
                    }
                    else
                    {
                        iUser = 0;
                        iPercent = 0;
                    }

                    //---------------------------------------------
                    // 1. DO MIN and %% DISTRIBUTION
                    //---------------------------------------------
                    if (cPercent)
                    {
                        if (iCellPercent < iColsPercent)
                        {
                            iCellPercent = iColsPercent;
                        }
                        if (xDistribute - xAdjust - iWidthMax > 0)
                        {
                            iNormalMaxForPercentColumns = MulDivQuick(xDistribute - xAdjust, iColsPercent, iCellPercent) - iPercentMax;
                            if (iNormalMaxForPercentColumns < 0)
                            {
                                iNormalMaxForPercentColumns = 0;
                            }
                            iNormalMax = MulDivQuick(xDistribute - xAdjust, iCellPercent - iColsPercent, iCellPercent) - (iWidthMax - iPercentMax);
                            if (iNormalMax < 0)
                            {
                                iNormalMax = 0;
                            }
                        }
                        if (xMin - xAdjust - iWidthMin > 0)
                        {
                            iNormalMinForPercentColumns = MulDivQuick(xMin - xAdjust, iColsPercent, iCellPercent) - iPercentMin;
                            if (iNormalMinForPercentColumns < 0)
                            {
                                iNormalMinForPercentColumns = 0;
                            }
                            iNormalMin = MulDivQuick(xMin - xAdjust, iCellPercent - iColsPercent, iCellPercent) - (iWidthMin - iPercentMin);
                            if (iNormalMin < 0)
                            {
                                iNormalMin = 0;
                            }
                        }
                    }
                    else
                    {
                        Assert (iPercentMin == 0 && iPercentMax == 0);

                        // if the spanned cell min width is greater then width of the spanned columns, then set the iNormalMin
                        if (xMin - xAdjust - iWidthMin > 0)
                        {
                            iNormalMin = xMin - xAdjust - iWidthMin;
                        }

                        // only adjust max if there is a normal column without the width set
                        if (xDistribute - xAdjust - iWidthMax > 0)
                        {
                            iNormalMax = xDistribute - xAdjust - iWidthMax;
                        }
                    }

                    iMinMaxDelta = (iWidthMax - iPercentMax) - (iWidthMin - iPercentMin);
                    if (iMinMaxDelta < 0)
                    {
                        iMinMaxDelta = 0;
                    }

                    //
                    // go thru the columns and adjust the widths
                    //
                    iMinSum = 0;
                    for (iCS = 0; iCS < cRealColSpan; iCS++)
                    {
                        iColMinWidthAdjust = 0; // Adjust the coulmn min by
                        iColMaxWidthAdjust = 0; // Adjust the coulmn max by

                        pColCalc = pColCalcBase + iCS;
                        if (pColCalc->IsDisplayNone())
                        {
                            continue;
                        }
                        fColWidthSpecified = pColCalc->IsWidthSpecified();
                        fColWidthSpecifiedInPercent = pColCalc->IsWidthSpecifiedInPercent();
                        fDoNormalMax = fColWidthSpecifiedInPercent? (iNormalMaxForPercentColumns != 0)
                                                                  : (iNormalMax != 0);      // Do normal Max distribution
                        fDoNormalMin = fColWidthSpecifiedInPercent? (iNormalMinForPercentColumns != 0)
                                                                  : (iNormalMin != 0);      // Do normal Min distribution
                        iOriginalColMin = pColCalc->_xMin;
                        iOriginalColMax = pColCalc->_xMax;

                        if ((iPercent && !fColWidthSpecifiedInPercent) || fColWidthSpecifiedInPercent)
                        {
                            // Do distribution of Min Max for %% columns
                            if (fColWidthSpecifiedInPercent)
                            {
                                iColPercent = pColCalc->GetPercentWidth();
                                xDistributeMax = iNormalMaxForPercentColumns + iPercentMax;
                                xDistributeMin = iNormalMinForPercentColumns + iPercentMin;
                                xPercent = iColsPercent;
                            }
                            else
                            {
                                // set percent if overall is percent width
                                Assert (cRealColSpan - cPercent > 0);
                                iColPercent =
                                    iWidthMax - iPercentMax
                                        ? MulDivQuick(iOriginalColMax, iPercent, iWidthMax - iPercentMax)
                                        : MulDivQuick(iPercent, 1, cRealColSpan - cPercent); // use MulDivQuick to round up...
                                pColCalc->SetPercentWidth(iColPercent);
                                xDistributeMax = iNormalMax + iWidthMax - iPercentMax;
                                xDistributeMin = iNormalMin + iWidthMin - iPercentMin;
                                xPercent = iPercent;
                            }

                            if (fDoNormalMax)
                            {
                                Assert (xPercent);
                                int iNewColMax = MulDivQuick(xDistributeMax, iColPercent, xPercent);
                                if (iNewColMax > pColCalc->_xMax)
                                {
                                    pColCalc->_xMax = iNewColMax;
                                }
                                fDoNormalMax = FALSE;
                            }
                            if (fDoNormalMin)
                            {
                                int iNewColMin = MulDivQuick(xDistributeMin, iColPercent, xPercent);
                                if (iNewColMin > pColCalc->_xMin)
                                {
                                    pColCalc->_xMin = iNewColMin;
                                }
                                fDoNormalMin = FALSE;
                            }
                            if (pColCalc->_xMin > pColCalc->_xMax)
                            {
                                pColCalc->_xMax = pColCalc->_xMin;
                            }
                            iNewPercentMax += pColCalc->_xMax;
                            cNewPercent++;
                        }

                        if (fDoNormalMin)
                        {
                            iColMinWidthAdjust =
                                fDoNormalMax
                                ? MulDivQuick(iOriginalColMax, iNormalMin, iWidthMax - iPercentMax)
                                : iMinMaxDelta
                                    ? MulDivQuick(iOriginalColMax - iOriginalColMin, iNormalMin, iMinMaxDelta)
                                    : iWidthMin - iPercentMin
                                        ? MulDivQuick(iOriginalColMin, iNormalMin, iWidthMin - iPercentMin)
                                        : MulDivQuick(iNormalMin, 1, cRealColSpan - cPercent); // use MulDivQuick to round up...
                        }

                        //if (!pColCalc->_fVirtualSpan)
                        //{
                            if (iCS > cRealColSpan - cVirtualSpan)
                            {
                                // Note: the first virtual column is not cet as being VirtualSpan
                                // Netscape: virtual columns min/max is calculated only once!
                                pColCalc->_fVirtualSpan = TRUE;
                                pColCalc->_pCell = pCell;
                            }
                            pColCalc->_xMin += iColMinWidthAdjust;
                        //}
                        if (pColCalc->_xMin > pColCalc->_xMax)
                        {
                            // make sure that by distrubuting extra into columns _xMin we didn't exeed _xMax
                            pColCalc->_xMax = pColCalc->_xMin;

                            //NETSCAPE: if the new MAX is greater and the column width was set from the cell,
                            //          don't propagate the user's width to the column.
                            if (pColCalc->_fWidthFromCell && pColCalc->IsWidthSpecifiedInPixel() && !puvWidth->IsSpecified())
                            {
                                // reset the column uvWidth
                                pColCalc->_fDontSetWidthFromCell = TRUE;
                                pColCalc->ResetWidth();
                            }
                            iExtraMax += pColCalc->_xMax - iOriginalColMax;
                        }
                        iMinSum += pColCalc->_xMin;
                    }

                    Assert (pColCalc->_xMin >=0 && pColCalc->_xMax >= 0);

                    // if the sum of all the column's Mins is less then the cell's min then
                    // adjust all the  columns...
                    if ((iMinSum -= xMin - xAdjust) < 0)
                    {
                        pColCalc->_xMin -= iMinSum; // adjust min of last column
                        // this will adjust col max to user setting
                        if (pColCalc->_xMin > pColCalc->_xMax)
                        {
                            iExtraMax += pColCalc->_xMin - pColCalc->_xMax;
                            pColCalc->_xMax = pColCalc->_xMin;
                        }
                    }
                    Assert (pColCalc->_xMin >=0 && pColCalc->_xMax >= 0);

                    pColCalcLast = pColCalc;
                    //---------------------------------------------
                    // 2. DO MAX DISTRIBUTION
                    //---------------------------------------------
                    // only adjust max if there is a normal column without the width set
                    iNormalMax = xDistribute - xAdjust - (iWidthMax + iExtraMax + iNewPercentMax - iPercentMax);
                    if (iNormalMax > 0)
                    {
                        //
                        // go thru the columns and adjust the widths
                        //
                        iMinSum = 0;    // just reusing variable, in this context it means the sum of adjustments
                        for (iCS = 0; iCS < cRealColSpan; iCS++)
                        {
                            iColMaxWidthAdjust = 0; // Adjust the coulmn max by

                            pColCalc = pColCalcBase + iCS;
                            if (pColCalc->IsDisplayNone())
                                continue;
                            fColWidthSpecified = pColCalc->IsWidthSpecified();
                            fColWidthSpecifiedInPercent = pColCalc->IsWidthSpecifiedInPercent();
                            fDoNormalMax = !fColWidthSpecifiedInPercent;
                            iOriginalColMax = pColCalc->_xMax;

                            if (fDoNormalMax)
                            {
                                // adjust pColCalc max later because it can effect min calculation down here...
                                iColMaxWidthAdjust =
                                  iWidthMax + iExtraMax - iPercentMax
                                    ? MulDivQuick(iOriginalColMax, iNormalMax, iWidthMax + iExtraMax - iPercentMax)
                                    : MulDivQuick(iNormalMax, 1, cRealColSpan - cNewPercent); // use MulDivQuick to round up...

                                // if (!pColCalc->_fVirtualSpan) // || pColCalc->_pCell == pCell)
                                // {
                                    pColCalc->_xMax += iColMaxWidthAdjust;
                                    iMinSum += iColMaxWidthAdjust;
                                    pColCalcLast = pColCalc;

                                    //NETSCAPE: if the new MAX is greater and the column width was set from the cell,
                                    //          don't propagate the user's width to the column.
                                    if (pColCalc->_fWidthFromCell && pColCalc->IsWidthSpecifiedInPixel() && !puvWidth->IsSpecified() && iColMaxWidthAdjust)
                                    {
                                        // reset the column uvWidth
                                        pColCalc->_fDontSetWidthFromCell = TRUE;
                                        pColCalc->ResetWidth();
                                    }
                                // }
                            }
                        }
                        if (iMinSum < iNormalMax && pColCalcLast)
                        {
                            // adjust last column
                            pColCalcLast->_xMax += iNormalMax - iMinSum;
                        }
                    }
                }
            }
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     SetCellPositions
//
//  Synopsis:   Set the final cell positions for the row
//
//--------------------------------------------------------------------------

void
CTableLayout::SetCellPositions(
    CTableCalcInfo * ptci,
    long        yTableTop)
{
    SIZEMODE        smMode = ptci->_smMode;
    CTableCell **   ppCell;
    CTableCell *    pCell;
    CTableCellLayout * pCellLayout;
    CDispNode *     pDispNode;
    int             cC;
    int             cCols = GetCols();
    CSize           sizeCell;
    int             iCellRowIndex;
    int             cCellRowSpan;
    CTableRow     * pRow = ptci->_pRow;
    CTableColCalc * pColCalc;
    CTable      *   pTable = ptci->_pTable;
    BOOL            fRTL = pTable->GetFirstBranch()->GetParaFormat()->HasRTL(TRUE);
    BOOL            fSetCellPositionOld;
    BOOL            fPositionParentTableGrid = IsGridAndMainDisplayNodeTheSame();

    CPoint          ptRow;

    Assert(pRow);

    CTableRowLayout * pRowLayout = ptci->_pRowLayout;

    Assert(pRowLayout);

    Assert(TestLock(CElement::ELEMENTLOCK_SIZING));

    PerfLog(tagTableSet, this, "+SetCellPositions");

    ptci->_smMode = SIZEMODE_SET;
    
    ptRow.x = pRowLayout->GetXProposed();
    ptRow.y = pRowLayout->GetYProposed();

    if (ptci->_pFFRow->_fPositioned)
    {
        if(   ptci->_pFFRow->_bPositionType == stylePositionrelative 
           && fPositionParentTableGrid)
        {
            // Note if the relatively positioned row will be positioned by the table grid display node,
            // then we need to subtract the border width, if the row will be positioned by the table node
            // which has a caption dsiplay node inside and the table grid node, then we are fine (since
            // border is included by the table grid node).
            if(!fRTL)
                ptRow.x -= _aiBorderWidths[BORDER_LEFT];
            else
                ptRow.x += _aiBorderWidths[BORDER_RIGHT];

            ptRow.y -= _aiBorderWidths[BORDER_TOP];
        }
        pRow->ZChangeElement(0, &ptRow);     // relative rows will be positioned under the MAIN table display node 
                                             // absolute rows will be positioned under the "BODY" display node 
    }

    ppCell = pRowLayout->_aryCells;
    for (cC = cCols, pColCalc = _aryColCalcs;
        cC > 0;
        cC--, ppCell++, pColCalc++)
    {
        if (pColCalc->IsDisplayNone())
            continue;
        pCell = Cell(*ppCell);
        if (pCell)
        {
            pCellLayout = pCell->Layout();

            cCellRowSpan =  pCell->RowSpan();
            
            if (IsSpanned(*ppCell))
            {
                iCellRowIndex = pCell->RowIndex();
                
                // if ends in this row and this is the first column of the cell
                if (iCellRowIndex + cCellRowSpan - 1 == pRowLayout->_iRow &&
                    pCellLayout->_iCol == cCols - cC)
                {
                    pCellLayout->GetSize(&sizeCell);
                    sizeCell.cy = ptRow.y + pRowLayout->_yHeight - pCellLayout->GetYProposed();
                    pCellLayout->_fSizeThis = TRUE;

                    fSetCellPositionOld = ptci->_fSetCellPosition;
                    ptci->_fSetCellPosition = TRUE;
                    pCellLayout->CalcSizeAtUserWidth(ptci, &sizeCell);
                    ptci->_fSetCellPosition = fSetCellPositionOld;

                    Assert( pCellLayout->_fChildHeightPercent
                        ||  pCellLayout->ContainsVertPercentAttr()
                        ||  pCellLayout->IsDisplayNone()
                        ||  (   pCellLayout->GetDisplay()
                            &&  (   sizeCell.cx == (pCellLayout->GetClientWidth() + pCellLayout->GetBorderAndPaddingWidth( ptci, TRUE ))
                                 || (pCellLayout->GetDisplay()->Printing() && pCellLayout->GetClientWidth() + pCellLayout->GetBorderAndPaddingWidth( ptci, TRUE ) - sizeCell.cx == 1)))); // Allow for rounding error on print scaling.
                }
            }
            else
            {
                if (ptci->_pFFRow->_fPositioned)
                {
                    // need to position cell relative to the row
                    // Note positioned row is including cell spacing, therefore we need to adjust Y position of the
                    // cell by vertical spacing
                    pCellLayout->SetYProposed(0 + _yCellSpacing);   // 0 - means relative to the row
                }
                else
                {
                    pCellLayout->SetYProposed(ptRow.y);
                }
                if (cCellRowSpan == 1)
                {
                    pCellLayout->GetSize(&sizeCell);

                    if (    sizeCell.cy != pRowLayout->_yHeight
                        ||  pCellLayout->_fChildHeightPercent
                        ||  pCellLayout->ContainsVertPercentAttr()
                        ||  pCell->GetFirstBranch()->GetParaFormat()->_bTableVAlignment == htmlCellVAlignBaseline  )
                    {
                        sizeCell.cy = pRowLayout->_yHeight;
                        pCellLayout->_fSizeThis = TRUE;
                        pCellLayout->CalcSizeAtUserWidth(ptci, &sizeCell);
                        Assert( pCellLayout->_fChildHeightPercent
                            ||  pCellLayout->ContainsVertPercentAttr()
                            ||  _cNestedLevel > SECURE_NESTED_LEVEL
                            ||  pRowLayout->IsDisplayNone()
                            ||  (   pCellLayout->GetDisplay()
                                &&  (   sizeCell.cx == (pCellLayout->GetClientWidth() + pCellLayout->GetBorderAndPaddingWidth( ptci, TRUE ))
                                     || (pCellLayout->GetDisplay()->Printing() && pCellLayout->GetClientWidth() + pCellLayout->GetBorderAndPaddingWidth( ptci, TRUE ) - sizeCell.cx == 1))));  // Allow for rounding error on print scaling.
                    }
                }

                if (    smMode == SIZEMODE_NATURAL
                    ||  smMode == SIZEMODE_SET
                    ||  smMode == SIZEMODE_FULLSIZE)
                {
                    const CFancyFormat * pFF = pCellLayout->GetFirstBranch()->GetFancyFormat();
                    CPoint               pt;

                    // Note: NOT POSITIONED cells located in the display tree under the table's GRID node 
                    // or under the rows
                    // Note: RELATIVELY positioned cells live in the display tree outside the table's grdi display node or
                    // under the rows if they are positioned

                    pt.x = pCellLayout->GetXProposed();
                    pt.y = pCellLayout->GetYProposed();
                    
                    // adjust the proposed position if the cell is not positioned
                    // or it is positioned and the row is not positioned and cell is directly 
                    // under the grid node.
                    // or if cell is relatively positioned and the row is also positioned
                    if (   !pFF->_fPositioned  
                        || (!ptci->_pFFRow->_fPositioned && fPositionParentTableGrid)
                        || ptci->_pFFRow->_fPositioned  )
                    {
                        if(!fRTL)
                            pt.x -= _aiBorderWidths[BORDER_LEFT];
                        else
                            pt.x += _aiBorderWidths[BORDER_RIGHT];

                        if (!ptci->_pFFRow->_fPositioned)
                            pt.y -= (_aiBorderWidths[BORDER_TOP] + yTableTop);
                    }

                    if (pFF->_fPositioned)
                    {
                        // relative cells will be positioned outside the table's grid display node (if the row is not positioned)
                        Assert (pFF->_bPositionType == stylePositionrelative);
                        pCell->ZChangeElement(0, &pt);
                    }
                    else
                    {
                        pDispNode = pCellLayout->GetElementDispNode();
                        if (pDispNode)
                        {
                            // NOTE:   We need some table-calc scratch space so that we don't have to use _ptProposed as the holder
                            //         of the suggested x/y (which is finalized with this call) (brendand)
                            //         Also, the scratch space needs to operate using the coordinates within the table borders since
                            //         the display tree translates taking borders into account (brendand)
                            pCellLayout->SetPosition(pt, TRUE);
                        }
                    }
                }
            }
        }
    }

    ptci->_smMode = smMode;

    PerfLog(tagTableSet, this, "-SetCellPositions");
}


//+----------------------------------------------------------------------------
//
//  Member:     SizeAndPositionCaption
//
//  Synopsis:   Size and position a CAPTION
//
//  Arguments:  ptci      - Current CCalcInfo
//              pCaption - CTableCaption to position
//              pt       - Point at which to position the caption
//
//-----------------------------------------------------------------------------
void
CTableLayout::SizeAndPositionCaption(
    CTableCalcInfo *ptci,
    CSize *         psize,
    CLayout **      ppLayoutSibling,
    CDispNode *     pDispNodeSibling,
    CTableCaption * pCaption,
    POINT *         ppt)
{
    CTableCellLayout *  pCaptionLayout = pCaption->Layout();

    Assert(psize);
    Assert(pDispNodeSibling);

    if ( pCaptionLayout )
    {
        pCaptionLayout->SetXProposed(ppt->x);

        if (!pCaptionLayout->NoContent() || (pCaptionLayout->_fContainsRelative || pCaptionLayout->_fAutoBelow))
        {
            SIZE    sizeCaption;

            sizeCaption.cx = psize->cx;
            sizeCaption.cy = 1;
            pCaptionLayout->CalcSize(ptci, &sizeCaption);

            pCaptionLayout->SetYProposed(ppt->y);
            psize->cy += sizeCaption.cy;

            if (    ptci->_smMode == SIZEMODE_NATURAL
                ||  ptci->_smMode == SIZEMODE_SET
                ||  ptci->_smMode == SIZEMODE_FULLSIZE)
            {
                HRESULT hr;

                hr = AddLayoutDispNode(pCaptionLayout,
                                       NULL,
                                       pDispNodeSibling,
                                       ppt,
                                       (pCaption->_uLocation == CTableCaption::CAPTION_TOP));

                if (    hr == S_OK
                    &&  ppLayoutSibling)
                {
                    *ppLayoutSibling = pCaptionLayout;
                }
            }

            ppt->y           += sizeCaption.cy;
        }
        else
        {
            pCaptionLayout->_sizeCell.cx     = psize->cx;
            pCaptionLayout->_sizeCell.cy     = 0;
            pCaptionLayout->SetYProposed(0);
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     AdjustRowHeights
//
//  Synopsis:   Distribute any extra height of a rowspan'd cell over the
//              spanned rows
//
//  NOTE: This routine will adjust the _sizeProposed for the table along
//        with the heights and sizes of the affected rows. However, it
//        depends upon the caller of this routine (e.g., CalculateRow) to
//        increase _sizeProposed by that of the last affected row.
//
//--------------------------------------------------------------------------

void
CTableLayout::AdjustRowHeights(
    CTableCalcInfo *ptci,
    CSize *         psize,
    CTableCell *    pCell)
{
    CSize       sizeCell;
    CTableRowLayout * pRowLayout;
    int         iRow  = pCell->RowIndex();
    int         cRows = pCell->RowSpan();
    int         cyRows;
    int         iRowCurrent;
    int         iRowLast;
    long        cyRowsActual;
    long        cyCell;
    CTable     *pTable = ptci->Table();

    const CHeightUnitValue * puvHeight;

    Assert(cRows > 1);

    // First, determine the height of the rowspan'd cell
    pCell->Layout()->GetSize(&sizeCell);

    puvHeight = (CHeightUnitValue *)&pCell->GetFirstBranch()->GetFancyFormat()->_cuvHeight;
    cyCell    = (!puvHeight->IsSpecified() || puvHeight->IsSpecifiedInPercent()
                        ? 0
                        : puvHeight->GetPixelHeight(ptci, pTable));
    cyRows    = max(sizeCell.cy, cyCell);

    // Next, determine the height of the spanned rows
    iRowLast     = iRow + cRows - 1;
    pRowLayout   = GetRow(iRowLast)->Layout();
    cyRowsActual = (pRowLayout->GetYProposed() + pRowLayout->_yHeight) - GetRow(iRow)->Layout()->GetYProposed();

    // Last, if the cell height is greater, distribute the difference over the spanned rows
    if (cyRows > cyRowsActual)
    {
        long    dyProposed = 0;
        long    dyHeight   = (cyRows - cyRowsActual);
        long    dyRow = 0;

        // Distribute the difference proportionately across affected rows
        for (iRowCurrent = iRow; iRowCurrent <= iRowLast; iRowCurrent++)
        {
            pRowLayout = GetRow(iRowCurrent)->Layout();
            pRowLayout->SetYProposed(pRowLayout->GetYProposed() + dyProposed);

            dyRow = MulDivQuick(pRowLayout->_yHeight, dyHeight, cyRowsActual);

            pRowLayout->_yHeight         += dyRow;
            dyProposed                   += dyRow;
        }

        // If the total height differs (due to round-off error),
        // use the last row to make up the difference
        // NETSCAPE: Navigator always uses the last row, even if it has zero height
        cyRowsActual = (pRowLayout->GetYProposed() + pRowLayout->_yHeight) - GetRow(iRow)->GetCurLayout()->GetYProposed();
        GetRow(iRowLast)->Layout()->_yHeight += cyRows - cyRowsActual;
        dyRow                      += cyRows - cyRowsActual;

        // Adjust total table height
        // (The last row is excluded since the caller of this routine adds its height to the table)
        psize->cy += dyHeight - dyRow;
        Assert(psize->cy == pRowLayout->GetYProposed());
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CalculateRow
//
//  Synopsis:   Calculate the cell heights and the row height
//
//  Returns:    TRUE if a did not fit in the determined column width
//              FALSE otherwise
//
//--------------------------------------------------------------------------

BOOL
CTableLayout::CalculateRow(
    CTableCalcInfo *    ptci,
    CSize *             psize,
    CLayout **          ppLayoutSibling,
    CDispContainer *    pDispNode)
{
    SIZEMODE        smMode    = ptci->_smMode;
    DWORD           grfLayout = ptci->_grfLayout;
    CTableCell **   ppCell;
    CTableCell *    pCell;
    CTableCellLayout * pCellLayout;
    CTableColCalc * pColCalc;
    int             cC, cColSpan, iCS;
    long            iWidth = 0;
    SIZE            sizeCell;
    int             cUnsizedCols;
    int             cCols = GetCols();
    BOOL            fCellSizeTooLarge  = FALSE;
    BOOL            fAdjustForRowspan  = FALSE;
    CTable      *   pTable = ptci->_pTable;

    Assert (pTable == Table());

    BOOL            fRTL = pTable->GetFirstBranch()->GetParaFormat()->HasRTL(TRUE);
    int             cCellRowSpan;
    int             iCellRowIndex;
    CLayout *       pLayoutSiblingCell = *ppLayoutSibling;
    BOOL            fSetNewSiblingCell = TRUE;
    BOOL            fPotentialDelayCalc = FALSE;
    CTableRow       * pRow = ptci->_pRow;

    Assert(pRow);

    CTableRowLayout * pRowLayout = ptci->_pRowLayout;

    Assert(pRowLayout);

    BOOL            fRowDisplayNone = pRowLayout->GetFirstBranch()->IsDisplayNone();
    
    // current offset in the row
    long            iLeft = 0;
    long            xPos;

        
    PerfLog(tagTableRow, this, "+CalculateRow");

    CElement::CLock   Lock(pTable, CElement::ELEMENTLOCK_SIZING);

    ptci->_smMode    = SIZEMODE_NATURAL;
    ptci->_grfLayout = ptci->_grfLayout & (LAYOUT_TASKFLAGS | LAYOUT_FORCE);

    //
    // Determine the number of sized columns (if any)
    //

    cUnsizedCols = cCols - _cSizedCols;

    // to calc baseline
    pRowLayout->_yBaseLine = -1;

    pRowLayout->_yHeight  = 0;
    // int iff = pRow->GetFirstBranch()->GetFancyFormatIndex();    // ensure compute format on the row.

    if (fRowDisplayNone)
        goto ExtractFromTree;

    if (ptci->_pFFRow->_fPositioned)
    {
        HRESULT hr = pRowLayout->EnsureDispNode(ptci, TRUE);
        if (!FAILED(hr))
        {
            // all the cells will be parented to the row's disp node container
            pDispNode = DYNCAST(CDispContainer, pRowLayout->GetElementDispNode());
            fSetNewSiblingCell = FALSE;
            pLayoutSiblingCell = NULL;
        }
    }


    if (IsFixedBehaviour())
    {
        if (pRowLayout->GetCells() < cCols)
        {
            pRowLayout->EnsureCells(cCols);
        }
    }

    Assert(pRowLayout->GetCells() == cCols);

    if ((IsFixedBehaviour() || _cSizedCols) && (pRow->_fAdjustHeight || pRow->_fHaveRowSpanCells))
    {   // if we have rowspan cells we can not be sure that the final rowSpan value is not 1, therefore we have to loop
        ppCell = pRowLayout->_aryCells;

        for (cC = 0; cC < cCols; cC++, ppCell++)
        {
            if (IsReal(*ppCell))
            {
                pCell = Cell(*ppCell);
                Assert (pCell);
                if (!pCell->Layout()->_fMinMaxValid && pCell->RowSpan() == 1)
                {
                    // adjust height of the row for specified height of the cell
                    pRowLayout->AdjustHeight(pCell->GetFirstBranch(), ptci, pTable); 
                }
            }
        }
    }
    if (pRowLayout->IsHeightSpecified())
    {
        if (!pRowLayout->IsHeightSpecifiedInPercent())
        {
            pRowLayout->_yHeight = pRowLayout->GetPixelHeight(ptci);
            // NOTE: DELAY RECALCED IS DISABLED due to the different problems in the display tree (we need to turn it ON in IE6)
            fPotentialDelayCalc  = FALSE; // IsFixed();
        }
    }

    if(!fRTL)
        xPos = _aiBorderWidths[BORDER_LEFT] + _xCellSpacing;
    else
        xPos = -(_aiBorderWidths[BORDER_RIGHT] + _xCellSpacing);

    pRowLayout->SetXProposed(xPos);
    pRowLayout->SetYProposed(fSetNewSiblingCell? 
                              psize->cy : 
                              psize->cy - _yCellSpacing);   // positioned rows include horsizontal and vertical cellSpacing

ExtractFromTree:
    
    ppCell = pRowLayout->_aryCells;

    for (cC = 0, pColCalc = _aryColCalcs;
         cC < cCols;
         cC++, ppCell++, pColCalc++)
    {
        if (!IsEmpty(*ppCell))
        {
            pCell = Cell(*ppCell);
            pCellLayout = pCell->Layout();

            if (pColCalc->IsDisplayNone() || fRowDisplayNone)
            {
                CDispNode *pDispNodeOld = pCell->Layout()->GetElementDispNode();
                if (!IsSpanned(*ppCell) && pDispNodeOld)
                {
                    pDispNodeOld->ExtractFromTree();
                }
                continue;
            }

            if (IsSpanned(*ppCell))
            {
                cCellRowSpan = pCell->RowSpan();
                iCellRowIndex = pCell->RowIndex();

                // if cell ends in this row adjust row height
                if (iCellRowIndex + cCellRowSpan - 1 == pRowLayout->_iRow &&
                    pCellLayout->_iCol == cC)
                {
                    //
                    // Skip the cell if it begins in this row
                    // (Since its size is applied when it is first encountered)
                    //

                    if (iCellRowIndex != pRowLayout->_iRow)
                    {
                        fAdjustForRowspan = TRUE;
                    }
#if DBG==1
                    //
                    // This must have been a COLSPAN'd cell, ROWSPAN should be equal to one
                    //

                    else
                    {
                        Assert(cCellRowSpan == 1);
                    }
#endif
                }

            }
            else
            {
                // Track if we run into a COLS at this table nesting level.
                BOOL fTableContainsCols = ptci->_fTableContainsCols;
                ptci->_fTableContainsCols = FALSE;

                cColSpan = pCell->ColSpan();

                // calc cell width
                iWidth = pColCalc->_xWidth;
                CTableColCalc *pColCalcTemp;

                for (iCS = 1; iCS < cColSpan; iCS++)
                {
                    pColCalcTemp = pColCalc + iCS;
                    if (pColCalcTemp->_xWidth)
                    {
                        iWidth  += pColCalcTemp->_xWidth;
                        if (!pColCalcTemp->_fVirtualSpan)
                        {
                            iWidth  += _xCellSpacing;
                        }
                    } // else pure virtual (cell was already adjusted for it).
                }

                //
                // If this is a fixed size cell, set its min/max values
                // (Since CalcSize(SIZEMODE_MMWIDTH) is not invoked on fixed size cells,
                //  these values may not have been set)
                //


                if (cC < _cSizedCols && !pCellLayout->_fMinMaxValid)
                {
                    pCellLayout->_xMin =
                    pCellLayout->_xMax = iWidth;
                    pCellLayout->_fMinMaxValid = TRUE;
                }

                // get cell height
                sizeCell.cx = iWidth;
                sizeCell.cy = 0;
                pCellLayout->_fContentsAffectSize = TRUE;
                pCellLayout->_fDelayCalc = FALSE;

                if (fPotentialDelayCalc && pCell->IsNoPositionedElementsUnder())
                {
                    sizeCell.cy = pRowLayout->_yHeight;
                    pCellLayout->EnsureDispNode(ptci, 0);
                    pCellLayout->_fContentsAffectSize = FALSE;
                    pCellLayout->_fDelayCalc = TRUE;
                }
                
                pCellLayout->CalcSizeAtUserWidth(ptci, &sizeCell);
                
                if (pCellLayout->_fForceMinMaxOnResize)
                {
                    _fForceMinMaxOnResize = TRUE;   // need to force min max on resize; bug #66432
                }

                //
                // If the sized cell is larger than the supplied width and it was
                // not min/max calculated, note the fact for our caller
                // (This can occur since sized columns take their width from user supplied
                //  values rather than the content of the cell)
                //

                // Cell overflow should only occur if we are dealing with fixed sized cells,
                // either at this level or in an embedded table.
                if ((ptci->_fTableContainsCols || cC < _cSizedCols) && sizeCell.cx > iWidth && !_fAlwaysMinMaxCells)
                {
                    pCellLayout->ResetMinMax();
                    fCellSizeTooLarge = TRUE;
                }

                // Restore potential table sibling's _fTableContainsCols.
                ptci->_fTableContainsCols |= fTableContainsCols || cC < _cSizedCols;

                //
                // 1. _yBaseLine on the row is used only for the cells with the specified baseline
                //    alignment.
                // 2. NETSCAPE: Baseline is taken from ALL the cells in the row
                //

                if (pCellLayout->_yBaseLine > pRowLayout->_yBaseLine)
                {
                    pRowLayout->_yBaseLine = pCellLayout->_yBaseLine;
                }


#if DBG == 1
                if (IsTagEnabled(tagTableCellSizeCheck))
                {
                    Assert(iWidth >= pCellLayout->GetDisplay()->GetWidth() + pCellLayout->GetBorderAndPaddingWidth( ptci, TRUE ));
                }
#endif

                if (    smMode == SIZEMODE_NATURAL
                    ||  smMode == SIZEMODE_SET
                    ||  smMode == SIZEMODE_FULLSIZE)
                {
                    const CFancyFormat * pFF = pCell->GetFirstBranch()->GetFancyFormat();
                    if (!pFF->_fPositioned)
                    {
                        pLayoutSiblingCell = AddLayoutDispNode(pCellLayout, pDispNode, pLayoutSiblingCell);
                    }
                }

                if(!fRTL)
                    xPos = pRowLayout->GetXProposed() + iLeft;
                else
                    xPos = pRowLayout->GetXProposed() - iLeft - iWidth;

                pCellLayout->SetXProposed(xPos);

                pCellLayout->SetYProposed(pRowLayout->GetYProposed());

                // if not spanned beyond this row use this height
                if (    pCell->RowSpan() == 1
                    &&  sizeCell.cy > pRowLayout->_yHeight
                    &&  !(  IsFixed()
                        &&  pRowLayout->IsHeightSpecifiedInPixel()))
                {
                    pRowLayout->_yHeight = sizeCell.cy;
                }
            }
        }

        if (pColCalc->_xWidth)
        {
            iLeft  += pColCalc->_xWidth;

            if (!pColCalc->_fVirtualSpan)
            {
                iLeft  += _xCellSpacing;
            }
        }
    }

    //
    // If any cells were too large, exit immediately
    // (The row will be re-sized again with the proper widths)
    //

    if (fCellSizeTooLarge)
        goto Cleanup;

    // NETSCAPE: In Navigator, empty rows are 1 pixel high
    if (!pRowLayout->_yHeight && !fRowDisplayNone)
    {
        pRowLayout->_yHeight = 1;
    }

    // NOTE: if there is baseline alignment in the row we DON'T NEED to recalculate the row height
    // since
    // NETSCAPE: NEVER grows the cells based on baseline alignment
    // If any rowspan'd cells ended in this row, adjust the row heights.
    //

    if (fAdjustForRowspan)
    {
        for (cC = 0, ppCell = pRowLayout->_aryCells;
             cC < cCols;
             cC++, ppCell++)
        {
            if (!IsEmpty(*ppCell))
            {
                if (IsSpanned(*ppCell))
                {
                    pCell = Cell(*ppCell);
                    pCellLayout = pCell->Layout();
                    cCellRowSpan = pCell->RowSpan();
                    iCellRowIndex = pCell->RowIndex();

                    if (   iCellRowIndex != pRowLayout->_iRow    // the cell starts in one of the previous rows, AND
                        && iCellRowIndex +  cCellRowSpan - 1 == pRowLayout->_iRow   // ends in this row, AND
                        && pCellLayout->_iCol == cC)              // it is a first column of the spanned cell
                    {
                        AdjustRowHeights(ptci, psize, pCell);
                    }
                }
            }
        }
    }

    pRowLayout->_fSizeThis  = FALSE;

    ptci->_smMode    = smMode;
    ptci->_grfLayout = grfLayout;

    if (fRowDisplayNone)
        goto Cleanup;

    if (pRowLayout->PercentHeight())
    {
        _fHavePercentageRow = TRUE;
    }

    if ( pRowLayout->_fAutoBelow && !pRowLayout->ElementOwner()->IsZParent() )
    {
         _fAutoBelow = TRUE;
    }

    if (fSetNewSiblingCell)
    {
        *ppLayoutSibling = pLayoutSiblingCell;
    }
    else
    {
        // need to set the size on the display node of the row
        CSize  sz(psize->cx - _aiBorderWidths[BORDER_RIGHT] - _aiBorderWidths[BORDER_LEFT], 
                  pRowLayout->_yHeight + _yCellSpacing + _yCellSpacing);
        if (pRow->IsDisplayNone())
            sz.cy = 0;
        pDispNode->SetSize(sz, FALSE);
        xPos = pRowLayout->GetXProposed();
        pRowLayout->SetXProposed(xPos - _xCellSpacing);
    }

Cleanup:
    PerfLog(tagTableRow, this, "-CalculateRow");
    return fCellSizeTooLarge;
}


//+-------------------------------------------------------------------------
//
//  Method:     CalculateRows
//
//  Synopsis:   Calculate the row heights and table height
//
//--------------------------------------------------------------------------

void
CTableLayout::CalculateRows(
    CTableCalcInfo * ptci,
    CSize *     psize,
    long        yTableTop)
{
    CTableRowLayout * pRowLayout;
    CTableRow       * pRow;
    CTable          * pTable = ptci->Table();
    int             cR, cRows = GetRows();
    int             iRow;
    int             iPercent, iP;
    long            iMul, iDiv;
    int             iDelta;
    long            iNormalMin, iUserMin;
    long            iNormal, iUser;
    long            yHeight, yDelta;
    long            yTableHeight;
    int             yTablePadding;
    int             cAdjust;
    BOOL            fUseAllRows;
    long            iExtra;
#if DBG == 1
    int             cLoop;
#endif
    int             cOutRows = 0;

    yTablePadding = 2 * _yBorder + cRows * _yCellSpacing + _yCellSpacing;

    // calc sum known % and known width

    iPercent = 0;
    iMul = 0;
    iDiv = 1;
    iUserMin = 0;
    iNormalMin = 0;

    for (cR = cRows, iRow = GetFirstRow();
        cR > 0;
        cR--, iRow = GetNextRow(iRow))
    {
        pRow = _aryRows[iRow];
        if (!pRow->_fCompleted)
        {
            Assert (!_fCompleted && IsFixedBehaviour());  // if row is not completed, table also should not be completed
            yTablePadding -= _yCellSpacing;
            cOutRows++;
            continue;
        }
        ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();
        if ((pRow->IsDisplayNone()  || ptci->_pFFRow->_bPositionType == stylePositionabsolute) && !pRow->_fCrossingRowSpan)
        {
            yTablePadding -= _yCellSpacing;
            cOutRows++;
            continue;
        }
        pRowLayout = pRow->Layout();
        yHeight = pRowLayout->_yHeight;
        if (pRowLayout->IsHeightSpecified())
        {
            if (pRowLayout->IsHeightSpecifiedInPercent())
            {
                iP = pRowLayout->GetPercentHeight();

                // remember max height/% ratio
                if (iP)
                {
                    if (yHeight * iDiv > iP * iMul)
                    {
                        iMul = yHeight;
                        iDiv = iP;
                    }
                }

                if (iPercent + iP > 100)
                {
                    iP = 100 - iPercent;
                    iPercent = 100;
                    pRowLayout->SetPercentHeight(iP);
                }
                else
                {
                    iPercent += iP;
                }
            }
            else
            {
                Assert(yHeight >= pRowLayout->GetPixelHeight(ptci));
                iUserMin += yHeight;
            }
        }
        else
        {
            iNormalMin += yHeight;
        }
    }

    iP = 100 - iPercent;
    if (iP < 0)
    {
        iP = 0;
    }

    CHeightUnitValue uvHeight = GetFirstBranch()->GetCascadedheight();

    // if we are calculating min/max and percent is set ignore user setting
    if (uvHeight.IsSpecified() && !(uvHeight.IsSpecifiedInPercent() &&
        (  ptci->_smMode == SIZEMODE_MMWIDTH
        || ptci->_smMode == SIZEMODE_MINWIDTH
        )))
    {
        yTableHeight = uvHeight.GetPercentSpecifiedHeightInPixel(ptci, pTable, _sizeParent.cy) - yTablePadding;
    }
    else
    {
        // if user height is not given back calculate it from the max height/% ratio
        if (iPercent)
        {
            // check if the remaining user and normal columns are requiring bigger percentage
            if (iP)
            {
                if ((iUserMin + iNormalMin) * iDiv > iP * iMul)
                {
                    iMul = iUserMin + iNormalMin;
                    iDiv = iP;
                }
            }
            yTableHeight = MulDivQuick(100, iMul, iDiv);
        }
        else
        {
            yTableHeight = iUserMin + iNormalMin;
        }
    }

    // if current height is already bigger we cannot do anything...
    if (psize->cy - yTableTop >= yTableHeight + yTablePadding)
    {
        return;
    }

    // cache width remaining for normal and user columns
    yHeight = MulDivQuick(iP, yTableHeight, 100);

    // distribute remaining percentage amongst normal and user rows
    if (iUserMin)
    {
        iUser = iUserMin;
        if (iNormalMin)
        {
            iNormal = iNormalMin;
            if (iUser + iNormal < yHeight)
            {
                iNormal = yHeight - iUser;
            }
        }
        else
        {
            iNormal = 0;
            if (iUser < yHeight)
            {
                iUser = yHeight;
            }
        }
    }
    else
    {
        iUser = 0;
        if (iNormalMin)
        {
            iNormal = iNormalMin;
            if (iNormal < yHeight)
            {
                iNormal = yHeight;
            }
        }
        else
        {
            iNormal = 0;
        }
    }

    iP = MulDivQuick(100, yTableHeight - iUser - iNormal, yTableHeight);

    psize->cy = yTableTop + _aiBorderWidths[BORDER_TOP] + _yCellSpacing;

    // distribute extra height
    iNormal -= iNormalMin;
    iUser   -= iUserMin;

    // remember how many can be adjusted
    cAdjust = 0;

    for (cR = cRows, iRow = GetFirstRow();
        cR > 0;
        cR--, iRow = GetNextRow(iRow))
    {
        pRow = _aryRows[iRow];
        pRowLayout = pRow->Layout();
        ptci->_pRow = pRow;
        ptci->_pRowLayout = pRowLayout;
        ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();

        if (!pRow->_fCompleted)
        {
            Assert (!_fCompleted && IsFixedBehaviour());  // if row is not completed, table also should not be completed
            continue;
        }
        if ((pRow->IsDisplayNone()  || ptci->_pFFRow->_bPositionType == stylePositionabsolute) && !pRow->_fCrossingRowSpan)
            continue;
        
        if (!pRowLayout->IsHeightSpecified() && iNormalMin)
        {
            yHeight = pRowLayout->_yHeight + MulDivQuick(iNormal, pRowLayout->_yHeight, iNormalMin);
        }
        else if (pRowLayout->IsHeightSpecifiedInPercent())
        {
            yHeight = iPercent
                        ? MulDivQuick(yTableHeight,
                                    MulDivQuick(iP, pRowLayout->GetPercentHeight(), iPercent),
                                    100)
                        : 0;
            if (yHeight < pRowLayout->_yHeight)
            {
                yHeight = pRowLayout->_yHeight;
            }
        }
        else if (iUserMin)
        {
            yHeight = pRowLayout->_yHeight + MulDivQuick(iUser, pRowLayout->_yHeight, iUserMin);
        }
        pRowLayout->_yHeightOld = pRowLayout->_yHeight;
        pRowLayout->_yHeight    = yHeight;

        if (ptci->_pFFRow->_bPositionType == stylePositionrelative)
        {
            CDispContainer *pDispNode = DYNCAST(CDispContainer, pRowLayout->GetElementDispNode());
            Assert (pDispNode);
            CSize  sz(psize->cx - _aiBorderWidths[BORDER_RIGHT] - _aiBorderWidths[BORDER_LEFT], 
                      yHeight + _yCellSpacing + _yCellSpacing);
            pRowLayout->SetYProposed(psize->cy - _yCellSpacing);
            pDispNode->SetSize(sz, FALSE);
        }
        else
        {
            pRowLayout->SetYProposed(psize->cy);
        }
        SetCellPositions(ptci, yTableTop);

        psize->cy += yHeight + _yCellSpacing;
        // we can adjust this row since it is more than min height
        if (yHeight > pRowLayout->_yHeightOld)
        {
            cAdjust++;
        }
    }

    // adjust for table border

    psize->cy += _aiBorderWidths[BORDER_BOTTOM];
#if DBG == 1
    cLoop = 0;
#endif

    // this is used to keep track of extra adjustment couldn't be applied
    iExtra = 0;

    if (cRows - cOutRows)
    {
      while ((yDelta = yTableHeight + yTablePadding - (psize->cy - yTableTop)) != 0)
      {
        fUseAllRows = FALSE;
        if (yDelta > 0)
        {
            if (cAdjust == 0)
            {
                // use all the rows if we add...
                cAdjust = cRows - cOutRows;
                fUseAllRows = TRUE;
            }
        }
        
        // distribute rounding error
        if(!cAdjust)
            break;

        iMul = yDelta / cAdjust;
        iDiv = yDelta % cAdjust;
        iDelta = iDiv > 0 ? 1 : iDiv < 0 ? -1 : 0;

        psize->cy = yTableTop + _aiBorderWidths[BORDER_TOP] + _yCellSpacing;

        // recalc cAdjust again
        cAdjust = 0;

        for (cR = cRows, iRow = GetFirstRow();
            cR > 0;
            cR--, iRow = GetNextRow(iRow))
        {
            pRow = _aryRows[iRow];
            pRowLayout = pRow->Layout();

            ptci->_pRow = pRow;
            ptci->_pRowLayout = pRowLayout;
            ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();

            if (!pRow->_fCompleted)
            {
                Assert (!_fCompleted && IsFixedBehaviour());  // if row is not completed, table also should not be completed
                continue;
            }
            if ((pRow->IsDisplayNone()  || ptci->_pFFRow->_bPositionType == stylePositionabsolute) && !pRow->_fCrossingRowSpan)
                continue;

            yHeight = pRowLayout->_yHeight;
            if (    yHeight > pRowLayout->_yHeightOld
                ||  (   yDelta > 0
                    &&  fUseAllRows))
            {
                yHeight += iMul + iDelta + iExtra;

                // if we went below min we have to adjust back...
                if (yHeight <= pRowLayout->_yHeightOld)
                {
                    iExtra  = yHeight - pRowLayout->_yHeightOld;
                    yHeight = pRowLayout->_yHeightOld;
                }
                else
                {
                    cAdjust++;
                    iExtra = 0;
                }

                iDiv -= iDelta;
                if (!iDiv)
                {
                    iDelta = 0;
                }
            }
            pRowLayout->_yHeightOld = pRowLayout->_yHeight;
            pRowLayout->_yHeight    = yHeight;

            if (ptci->_pFFRow->_bPositionType == stylePositionrelative)
            {
                CDispContainer *pDispNode = DYNCAST(CDispContainer, pRowLayout->GetElementDispNode());
                Assert (pDispNode);
                CSize  sz(psize->cx - _aiBorderWidths[BORDER_RIGHT] - _aiBorderWidths[BORDER_LEFT], 
                          yHeight + _yCellSpacing + _yCellSpacing);
                pRowLayout->SetYProposed(psize->cy - _yCellSpacing);
                pDispNode->SetSize(sz, FALSE);
            }
            else
            {
                pRowLayout->SetYProposed(psize->cy);
            }

            SetCellPositions(ptci, yTableTop);

            psize->cy += yHeight + _yCellSpacing;
        }

        // adjust for table border
        psize->cy += _aiBorderWidths[BORDER_BOTTOM];

#if DBG == 1
        cLoop++;
        Assert(cLoop < 5);
#endif
      }   // end of (while) loop
      

    }
    else
    {
        psize->cy += yTableHeight;
    }// end of if (cRows)

    Assert(iExtra == 0);
    Assert(psize->cy - yTableTop == yTableHeight + yTablePadding);
}


//+-------------------------------------------------------------------------
//
//  Method:     CalculateColumns
//
//  Synopsis:   Calculate column widths and table width
//
//--------------------------------------------------------------------------

void
CTableLayout::CalculateColumns(
    CTableCalcInfo * ptci,
    CSize *     psize)
{
    CTableColCalc * pColCalc;
    int     cC, cCols = GetCols();
    int     iPercentColumn, iPercent, iP;
    long    iMul, iDiv, iDelta;
    int     iPercentMin;
    long    iUserMin, iUserMax;
    long    iMin, iMax;
    long    iWidth;
    long    iNormal, iUser;
    BOOL    fUseMax = FALSE, fUseMin = FALSE, fUseMaxMax = FALSE;
    BOOL    fUseUserMax = FALSE, fUseUserMin = FALSE, fUseUserMaxMax = FALSE;
    BOOL    fSubtract = FALSE, fUserSubtract = FALSE;
    long    xTableWidth, xTablePadding;
    int     cAdjust;
    long    iExtra;
    BOOL    fUseAllColumns;
    CTable * pTable = ptci->Table();
    int     cDisplayNoneCols = 0;

#if DBG == 1
    int cLoop;
#endif

    PerfLog(tagTableColumn, this, "+CalculateColumns");

    xTablePadding = _aiBorderWidths[BORDER_RIGHT] + _aiBorderWidths[BORDER_LEFT] + _cNonVirtualCols * _xCellSpacing + _xCellSpacing;

    //
    // first calc sum known percent, user set and 'normal' widths up
    //
    iPercent = 0;
    iPercentMin = 0;
    iUserMin = iUserMax = 0;
    iMin = iMax = 0;

    //
    // we also keep track of the minimum width-% ratio necessary to
    // display the table with the columns at max width and the right
    // percent value
    //
    iMul = 0;
    iDiv = 1;

    //
    // Keep track of the first column which introduces a percentage width
    //

    iPercentColumn = INT_MAX;
    _fHavePercentageCol = FALSE;

    for (cC = cCols, pColCalc = _aryColCalcs;
        cC > 0;
        cC--, pColCalc++)
    {
        if (pColCalc->IsDisplayNone())
        {
            cDisplayNoneCols++;
            continue;
        }
        if (pColCalc->IsWidthSpecified())
        {
            if (pColCalc->IsWidthSpecifiedInPercent())
            {
                _fHavePercentageCol = TRUE;
                if (iPercentColumn > (cCols - cC))
                {
                    iPercentColumn = cCols - cC;
                }

                iP = pColCalc->GetPercentWidth();

                if (iP < 0)
                {
                    iP = 0;
                }
                // if we are over 100%, cut it back
                if (iPercent + iP > 100)
                {
                    iP = 100 - iPercent;
                    iPercent = 100;
                    pColCalc->SetPercentWidth(iP);
                }
                else
                {
                    iPercent += iP;
                }

                // remember max width/% ratio
                if (iP == 0)
                {
                    iP = 1; // at least non empty cell should get 1%
                }
                if (pColCalc->_xMax * iDiv > iP * iMul)
                {
                    iMul = pColCalc->_xMax;
                    iDiv = iP;
                }

                iPercentMin += pColCalc->_xMin;
            }
            else
            {
                iUserMax += pColCalc->_xMax;
                iUserMin += pColCalc->_xMin;
            }
        }
        else
        {
            Assert (pColCalc->_xMax >= 0 && pColCalc->_xMin >=0 );
            iMax += pColCalc->_xMax;
            iMin += pColCalc->_xMin;
        }
    }

    // iP is what remained left from the 100%
    iP = 100 - iPercent;
    Assert (iP >= 0);

    CWidthUnitValue uvWidth = GetFirstBranch()->GetCascadedwidth();

    //
    // If COLS was specified and there one or more columns percentage sized columns,
    // then default the table width to 100%
    // (When all columns are of fixed size, their sizes take precedence over any
    //  explicitly specified table width. Additionally, normal table sizing should
    //  be used if the only non-fixed size columns are those outside the range
    //  specified by COLS.)
    //

    if (!uvWidth.IsSpecified() && _cSizedCols > iPercentColumn)
    {
        uvWidth.SetPercent(100);
    }

    //
    // NS/IE compatibility, any value <= 0 is treated as <not present>
    //

    if (uvWidth.GetUnitValue() <= 0)
    {
        uvWidth.SetNull();
    }

    //
    // if uvWidth is set we use that value except when we are being called to
    // calculate for min/max and the width is percent since the parent information
    // is bogus then
    //
    if (uvWidth.IsSpecified() && !(uvWidth.IsSpecifiedInPercent() &&
        (  ptci->_smMode == SIZEMODE_MMWIDTH
        || ptci->_smMode == SIZEMODE_MINWIDTH
        )))
    {
        xTableWidth = uvWidth.GetPercentSpecifiedWidthInPixel(ptci, pTable, _sizeParent.cx);
        if (xTableWidth < _sizeMin.cx)
        {
            xTableWidth = _sizeMin.cx;
        }
        // if we want to limit the tabble width, then
        //        if (xTableWidth > MAX_TABLE_WIDTH)
        //        {
        //            xTableWidth = MAX_TABLE_WIDTH;
        //        }
    }
    else
    {
        // if user width is not given back calculate it from the max width/% ratio
        if (iPercent)
        {
            // check if the remaining user and normal columns are requiring bigger ratio
            if (iP)
            {
                if ((iUserMax + iMax) * iDiv> iP * iMul)
                {
                    iMul = iUserMax + iMax;
                    iDiv = iP;
                }
            }

            //
            // if there is percentage left or there are only percentage columns use the ratio
            // to back-calculate the table width
            //
            if (iP || (iUserMax + iMax) == 0)
            {
                // iP > 0 means the total percent specified columns = 100 - iP
                xTableWidth = MulDivQuick(100, iMul, iDiv) + xTablePadding;
                if (xTableWidth > _sizeParent.cx)
                {
                    xTableWidth = _sizeParent.cx;
                }
            }
            else
            {
                // otherwise use parent width
                xTableWidth = _sizeParent.cx;
            }
            if (xTableWidth < _sizeMin.cx)
            {
                xTableWidth = _sizeMin.cx;
            }
        }
        else
        {
            if (_sizeMax.cx < _sizeParent.cx)
            {
                // use max value if that smaller the parent size
                xTableWidth = _sizeMax.cx;
            }
            else if (_sizeMin.cx > _sizeParent.cx)
            {
                // have to use min if that is bigger the parent
                xTableWidth = _sizeMin.cx;
            }
            else
            {
                // use parent between min and max
                xTableWidth = _sizeParent.cx;
            }
        }
    }

    // if there are no columns, set proposed and return
    if (!cCols)
    {
        psize->cx = xTableWidth;
        return;
    }

    //
    // If all columns are of fixed size, set the table width to the sum and return
    //

    if (_fCols &&
        (!uvWidth.IsSpecified() || (xTableWidth <= (iUserMin + iMin + xTablePadding))))
    {
        Assert(iPercent == 0);
        Assert(iMax == iMin);
        Assert(iUserMax == iUserMin);
        // set the width of the columns
        for (cC = cCols, pColCalc = _aryColCalcs;
            cC > 0;
            cC--, pColCalc++)
        {
            pColCalc->_xWidth = pColCalc->_xMin;
        }
        psize->cx = iUserMin + iMin + xTablePadding;
        return;
    }

    // subtract padding width which contains border and cellspacing
    if (xTableWidth >= xTablePadding)
    {
        xTableWidth -= xTablePadding;
    }

    if (iMax + iUserMax)
    {
        // cache width remaining for normal and user columns over percent columns (iWidth)
        iWidth = MulDivQuick(iP, xTableWidth, 100);
        if (iWidth < iUserMin + iMin)
        {
            iWidth = iUserMin + iMin;
        }
        if (iWidth > xTableWidth - iPercentMin)
        {
            iWidth = xTableWidth - iPercentMin;
        }
    }
    else
    {
        // all widths is for percent columns
        iWidth = 0;
    }

    //
    // distribute remaining width amongst normal and user columns
    // first try to use max width for user columns and normal columns
    //
    if (iUserMax)
    {
        iUser = iUserMax;
        if (iUser > iWidth)
        {
            iUser = iWidth;
        }
        if (iMax)
        {
            iNormal = iMin;
            if (iUser + iNormal <= iWidth)
            {
                iNormal = iWidth - iUser;
            }
            else
            {
                iUser = iUserMin;
                if (iUser + iNormal <= iWidth)
                {
                    iUser = iWidth - iNormal;
                }
            }
        }
        else
        {
            iNormal = 0;
            if (iUser < iWidth)
            {
                iUser = iWidth;
            }
        }
    }
    else
    {
        iUser = 0;
        if (iMax)
        {
            iNormal = iMin;
            if (iNormal < iWidth)
            {
                iNormal = iWidth;
            }
        }
        else
        {
            iNormal = 0;
        }
    }

    if (iNormal > iMax)
    {
        fUseMaxMax = TRUE;
    }
    else if (iNormal == iMax)
    {
        fUseMax = TRUE;
    }
    else if (iNormal == iMin)
    {
        fUseMin = TRUE;
    }
    else if (iNormal < iMax)
    {
        fSubtract = TRUE;
    }

    if (iUser > iUserMax)
    {
        fUseUserMaxMax = TRUE;
    }
    else if (iUser == iUserMax)
    {
        fUseUserMax = TRUE;
    }
    else if (iUser == iUserMin)
    {
        fUseUserMin = TRUE;
    }
    else if (iUser < iUserMax)
    {
        fUserSubtract = TRUE;
    }

    // calculate real percentage of percent columns in the table now using the final widths
    iP = xTableWidth ? MulDivQuick(100, xTableWidth - iUser - iNormal, xTableWidth)
                     : 0;

    // start with the padding
    psize->cx = xTablePadding;

    // remember how many columns can be adjusted
    cAdjust = 0;

    //
    // now go and calculate column widths by distributing the extra width over
    // the min width or subtracting the extra width from max
    //
    for (cC = cCols, pColCalc = _aryColCalcs;
        cC > 0;
        cC--, pColCalc++)
    {
        if (pColCalc->IsDisplayNone())
            continue;

        if (!pColCalc->IsWidthSpecified())
        {
            // adjust normal column by adding to min or subtracting from max
            pColCalc->_xWidth =
                fSubtract ?
                    pColCalc->_xMax - MulDivQuick(pColCalc->_xMax - pColCalc->_xMin,
                        iMax - iNormal,
                        iMax - iMin) :
                fUseMaxMax?
                    pColCalc->_xMax + MulDivQuick(pColCalc->_xMax, iNormal - iMax, iMax) :
                fUseMax ?
                    pColCalc->_xMax :
                fUseMin ?
                    pColCalc->_xMin :
                iMax ?
                    pColCalc->_xMin +
                    MulDivQuick(pColCalc->_xMax, iNormal - iMin, iMax) :
                    0;
        }
        else if (pColCalc->IsWidthSpecifiedInPercent())
        {
            //
            // if percent first calculate the width from the percent
            //
            iWidth = iPercent ?
                MulDivQuick(xTableWidth,
                    MulDivQuick(iP, pColCalc->GetPercentWidth(), iPercent),
                    100) :
                0;
            //
            // make sure it is over the min width
            //
            iWidth -= pColCalc->_xMin;
            if (iWidth < 0)
            {
                iWidth = 0;
            }
            pColCalc->_xWidth = pColCalc->_xMin + iWidth;
        }
        else
        {
            // adjust user column by adding to min or subtracting from max
            pColCalc->_xWidth =
                fUserSubtract   // table needs to be (iUserMax - iUser) pixels shorter, so subtract pixels
                                // from columns proportionally to (pColCalc->_xMax - pColCalc->_xMin)
                    ? pColCalc->_xMax - MulDivQuick(pColCalc->_xMax - pColCalc->_xMin,
                                                    iUserMax - iUser,
                                                    iUserMax - iUserMin)
                    :
                fUseUserMaxMax
                    ? pColCalc->_xMax + MulDivQuick(pColCalc->_xMax, iUser - iUserMax, iUserMax)
                    :
                fUseUserMax
                    ? pColCalc->_xMax
                    :
                fUseUserMin
                    ? pColCalc->_xMin
                    :
                iUserMax
                    ? pColCalc->_xMin + MulDivQuick(pColCalc->_xMax,
                                                    iUser - iUserMin,
                                                    iUserMax)
                    : 0;
        }
        Assert(pColCalc->_xWidth >= pColCalc->_xMin);

        // we can adjust this col since it is more than min width
        if (pColCalc->_xWidth > pColCalc->_xMin)
        {
            cAdjust++;
        }
        psize->cx += pColCalc->_xWidth;
    }

    // this is used to keep track of extra adjustment couldn't be applied
    iExtra = 0;

    // distribute rounding error

#if DBG == 1
    cLoop = 0;
#endif

    while (xTableWidth && (iWidth = xTableWidth + xTablePadding - psize->cx) != 0)
    {
        fUseAllColumns = FALSE;
        if (iWidth > 0)
        {
            if (cAdjust == 0)
            {
                // use all the cols if we add...
                cAdjust = cCols - cDisplayNoneCols;
                if (!cAdjust)
                    break;
                fUseAllColumns = TRUE;
            }
        }
        Assert(cAdjust);
        iMul = iWidth / cAdjust;                    // iMul is the adjustment for every column
        iDiv = iWidth % cAdjust;                    // left-over adjustment for all the columns
        iDelta = iDiv > 0 ? 1 : iDiv < 0 ? -1 : 0;  // is the +/- 1 pixel that is added to every column
        iExtra = 0;

        // start with the padding
        psize->cx = xTablePadding;

        // recalc cAdjust again
        cAdjust = 0;

        for (cC = cCols, pColCalc = _aryColCalcs;
            cC > 0;
            cC--, pColCalc++)
        {
            if (pColCalc->IsDisplayNone())
                continue;

            if (pColCalc->_xWidth > pColCalc->_xMin || (iWidth > 0 && fUseAllColumns))
            {
                pColCalc->_xWidth += iMul + iDelta + iExtra;
                // if we went below min we have to adjust back...

                if (pColCalc->_xWidth <= pColCalc->_xMin)
                {
                    iExtra = pColCalc->_xWidth - pColCalc->_xMin;
                    pColCalc->_xWidth = pColCalc->_xMin;
                }
                else
                {
                    iExtra = 0;
                    cAdjust++;
                }

                iDiv -= iDelta;
                if (!iDiv)
                {
                    iDelta = 0; // now left-over for every column is 0
                }
            }
            psize->cx += pColCalc->_xWidth;
        }
#if DBG == 1
        cLoop++;
        Assert(cLoop < 5);
#endif
    }

    Assert(!xTableWidth || (xTableWidth + xTablePadding == psize->cx) || (cCols == cDisplayNoneCols));
    PerfLog(tagTableColumn, this, "-CalculateColumns");
}


//+-------------------------------------------------------------------------
//
//  Method:     CalculateLayout
//
//  Synopsis:   Calculate cell layout in the table
//
//--------------------------------------------------------------------------

void
CTableLayout::CalculateLayout(
    CTableCalcInfo * ptci,
    CSize *     psize,
    BOOL        fWidthChanged,
    BOOL        fHeightChanged)
{
    SIZEMODE         smMode = ptci->_smMode;
    CSize            sizeTable;
    CTableCaption ** ppCaption;
    CDispContainer * pDispNodeTableOuter;
    CDispContainer * pDispNodeTableInner;
    CLayout *        pLayoutSiblingCell;
    int              cR, iR;
    int              cC;
    int              yCaption;
    long             yTableTop;
    BOOL             fRedoMinMax    = FALSE;
    BOOL             fTopCaption    = FALSE;
    BOOL             fBottomCaption = FALSE;
    BOOL             fIncrementalRecalc = !fWidthChanged && !fHeightChanged && IsFixedBehaviour();
    CTableRow      * pRow = NULL;
    int              cRows = GetRows();
    int              cRowsIncomplete = 0;
    CTable         * pTable = ptci->Table();
    BOOL             fForceMinMax    = fWidthChanged && (_fHavePercentageInset || _fForceMinMaxOnResize);

    CElement::CLock   Lock(pTable, CElement::ELEMENTLOCK_SIZING);

    Assert (CanRecalc());

    if (fIncrementalRecalc)
    {
        if (pTable->IsDatabound())
        {
            // do incremental recalc only when in a process of fetching rows and populating the table
            fIncrementalRecalc = pTable->_readyStateTable == READYSTATE_INTERACTIVE;
        }
        else
        {
            // do incremental recalc only when loading is not complete and there are new rows
            fIncrementalRecalc = _cCalcedRows != (cRows - (_pFoot? _pFoot->_cRows : 0));
            if (!fIncrementalRecalc)
            {
                // if it is a fixed style table and there are no new rows to claculate, just return
                return;
            }
        }
    }

#ifdef  TABLE_PERF
    ::StartCAP();
#endif

    PerfLog(tagTableLayout, this, "+CalculateLayout");

    if (!_aryCaptions.Size() && !cRows) // if the table is empty
    {
        _sizeMax =
        _sizeMin = 
        *psize   = 
        sizeTable = g_Zero.size;
        _fZeroWidth = TRUE;
    }
    else
    {    
        // reset perentage based rows
        _fHavePercentageRow = FALSE;

        // Create measurer.
        CLSMeasurer me;

        ptci->_pme = &me;

        //
        // Determine the cell widths/heights and row heights
        //

        do
        {
            // calculate min/max only if that information is dirty or forced to do it
            // NOTE:   It would be better if tables, rows, and cells all individually tracked
            //         their min/max dirty state through a flag (as cells do now). This would
            //         allow CalculateMinMax to be more selective in the rows/cells it
            //         processed. (brendand)
            if (_sizeMin.cx <= 0 || _sizeMax.cx <= 0 || (ptci->_grfLayout & LAYOUT_FORCE) || fRedoMinMax || fForceMinMax)
            {
                TraceTag((tagTableCalc, "CTableLayout::CalculateLayout - calling CalculateMinMax (0x%x)", pTable));
                _fAlwaysMinMaxCells = _fAlwaysMinMaxCells || fRedoMinMax;

                // if it is an incremental recal, do min max calculation only for the first time
                if (!fIncrementalRecalc || (_cCalcedRows == 0))
                {
                    // do min max
                    CalculateMinMax(ptci);
                    if (_sizeMin.cx < 0)
                    {
                        return; // it means that we have failed incremental recalc, 
                                // because we have failed to load history
                    }
                }
            }

            _fForceMinMaxOnResize = FALSE;   // initialize to FALSE; (TRUE - means need to force min max on resize; bug #66432)

            //
            // Ensure display tree nodes exist
            // (Only do this the first time through the loop)
            //

            if (!fRedoMinMax)
            {
                EnsureTableDispNode(ptci, (ptci->_grfLayout & LAYOUT_FORCE));
            }

            pDispNodeTableOuter  = GetTableOuterDispNode();
            pDispNodeTableInner      = GetTableInnerDispNode();
            pLayoutSiblingCell = NULL;

            // Force is only needed during min/max calculations; or if there were no MinMax calc performed
            if (!IsFixedBehaviour())
                ptci->_grfLayout = (ptci->_grfLayout & ~LAYOUT_FORCE);

            psize->cy = 0;

            // if it is an incremental recal, calculate columns only for the first time
            if (!fIncrementalRecalc || (_cCalcedRows == 0))
            {
                psize->cx = 0;

                //
                // first calc columns and table width
                // the table layout is defined by the columns widths and row heights which in
                // turn will finalize the cell widths and heights
                //


                CalculateColumns(ptci, psize);
            }
            else
            {
                // Make sure psize.cx is set during incremental recalc.
                GetSize(psize);
            }

            //
            // Size top CAPTIONs and TCs
            // NOTE: Position TCs on top of all CAPTIONs
            //

            {
                CPoint  pt(0, 0);

                ptci->_smMode = SIZEMODE_NATURAL;

                for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
                     cC > 0;
                     cC--, ppCaption++)
                {
                    if ((*ppCaption)->Tag() == ETAG_TC)
                    {
                        Assert((*ppCaption)->_uLocation == CTableCaption::CAPTION_TOP);
                        SizeAndPositionCaption(ptci, psize, pDispNodeTableOuter, *ppCaption, &pt);
                    }
                    else
                    {
                        if ((*ppCaption)->_uLocation == CTableCaption::CAPTION_TOP)
                        {
                            fTopCaption = TRUE;
                        }
                        else
                        {
                            fBottomCaption = TRUE;
                        }
                    }
                }

                yCaption = pt.y;
            }

            if (fTopCaption)
            {
                for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
                     cC > 0;
                     cC--, ppCaption++)
                {
                    CPoint  pt(0, yCaption);

                    if (    (*ppCaption)->Tag() != ETAG_TC
                        &&  (*ppCaption)->_uLocation == CTableCaption::CAPTION_TOP)
                    {
                        SizeAndPositionCaption(ptci, psize, pDispNodeTableOuter, *ppCaption, &pt);
                    }

                    yCaption = pt.y;
                }
            }
            ptci->_smMode = smMode;

            // set _sizeParent in tci

            ptci->_sizeParent = *psize;

            // remember table top

            yTableTop = psize->cy;

            //
            // Initialize the table height to just below the captions and top border
            //

            psize->cy += _aiBorderWidths[BORDER_TOP] + _yCellSpacing;

            //
            // Calculate natural cell/row sizes
            // NOTE: Since the COLS attribute assigns column widths without examining every cell
            //       of every row, it is possible that a cell will contain content which cannot
            //       fit within the assigned size. If that occurs, all rows/cells before that
            //       point must be sized again to the larger width to prevent overflow.
            //

            for (cR = cRows, iR = GetFirstRow();
                 cR > 0;
                 cR--, iR = GetNextRow(iR))
            {
                pRow = _aryRows[iR];
                if (!pRow->_fCompleted)
                {
                    Assert (!_fCompleted && IsFixedBehaviour());  // if row is not completed, table also should not be completed
                    continue;
                }
                ptci->_pRow = pRow;
                ptci->_pRowLayout = pRow->Layout();
                ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();
                if (!fIncrementalRecalc || iR >= _cCalcedRows || IsFooterRow(iR))
                {
                    // calculate row (in case of incremental recalc - calculate only new rows)
                    fRedoMinMax = CalculateRow(ptci, psize, &pLayoutSiblingCell, pDispNodeTableInner)
                                &&  !_fAlwaysMinMaxCells;

                    // stop calculating rows when need to redo MinMax (since the cell in the row was too large to fit).
                    if (fRedoMinMax)
                        break;
                }

                Assert (ptci->_pRow == pRow);
                if ((pRow->IsDisplayNone()  || ptci->_pFFRow->_bPositionType == stylePositionabsolute) && !pRow->_fCrossingRowSpan)
                    continue;

                psize->cy += ptci->_pRowLayout->_yHeight + _yCellSpacing;
            }

            //
            // If any cells proved too large for the pre-determined size,
            // force a min/max recalculation
            // NOTE: This should only occur when the COLS attribute has been specified
            //

            Assert(!fRedoMinMax || _cSizedCols || ptci->_fTableContainsCols);

        //
        // If a cell within a row returned a width greater than its minimum,
        // force a min/max pass over all cells (if not previously forced)
        // (When COLS is specified, not all cells are min/max'd - If the page author
        //  included content which does not fit in the specified width, this path
        //  is taken to correct the table)
        //

        } while (fRedoMinMax);

        // set the positions of all cells
        for (cR = cRows, iR = GetFirstRow();
             cR > 0;
             cR--, iR = GetNextRow(iR))
        {
            pRow = _aryRows[iR];
            if (!pRow->_fCompleted)
            {
                Assert (!_fCompleted && IsFixedBehaviour());    // if row is not completed, table also should not be completed
                Assert ( cR == 1 + (_pFoot? _pFoot->_cRows: 0) );   // last row
                cRowsIncomplete++;
                continue;
            }
            if (!fIncrementalRecalc || iR >= _cCalcedRows || IsFooterRow(iR))
            {
                ptci->_pRow = pRow;
                ptci->_pRowLayout = pRow->Layout();
                ptci->_pFFRow = pRow->GetFirstBranch()->GetFancyFormat();

                SetCellPositions(ptci, yTableTop);
            }
        }

        // adjust for table border

        psize->cy += _aiBorderWidths[BORDER_BOTTOM];

        // adjust table height if necessary

        if (!GetFirstBranch()->GetCascadedheight().IsNull() || _fHavePercentageRow)
        {
            CalculateRows(ptci, psize, yTableTop);
        }

        //
        // Save the size of the table (excluding CAPTIONs)
        //

        sizeTable.cx = psize->cx;
        sizeTable.cy = psize->cy - yTableTop;

        //
        // Position the display node which holds the cells
        //

        if (_fHasCaptionDispNode)
        {
            if(!pDispNodeTableOuter->IsRightToLeft())
                pDispNodeTableOuter->SetPosition(CPoint(0, yTableTop));
            else
                pDispNodeTableOuter->SetPositionTopRight(CPoint(0, yTableTop));
        }

        //
        // Size bottom CAPTIONs
        //

        if (fBottomCaption)
        {
            CPoint          pt(0, psize->cy);
            BOOL            fInsertedBottomCaption = FALSE;
            CLayout *       pLayoutSiblingCaption = NULL;

            ptci->_smMode = SIZEMODE_NATURAL;

            for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
                 cC > 0;
                 cC--, ppCaption++)
            {
                if ((*ppCaption)->_uLocation == CTableCaption::CAPTION_BOTTOM)
                {
                    Assert((*ppCaption)->Tag() != ETAG_TC);

                    if (fInsertedBottomCaption)
                    {
                        SizeAndPositionCaption(ptci, psize, &pLayoutSiblingCaption, *ppCaption, &pt);
                    }
                    else
                    {
                        SizeAndPositionCaption(ptci, psize, &pLayoutSiblingCaption, pDispNodeTableOuter, *ppCaption, &pt);
                        fInsertedBottomCaption = pLayoutSiblingCaption != NULL;
                    }
                }
            }
            ptci->_smMode = smMode;
        }

        if (_pAbsolutePositionCells)
        {
            int               cCells;
            CTableCell      **ppCell;
            CPoint          pt(0,0);
            for (cCells = _pAbsolutePositionCells->Size(), ppCell = *_pAbsolutePositionCells ;  cCells > 0; cCells--, ppCell++)
            {
                CalcAbsolutePosCell(ptci, (*ppCell));
                (*ppCell)->ZChangeElement(0, &pt);
            }
        }
    }

    // cache sizing/recalc data
    _cDirtyRows = 0;
    _cCalcedRows = cRows - cRowsIncomplete; // cache the number of rows that were calculated (needed for incremental recalc)
    if (_pFoot)
    {
        _cCalcedRows -= _pFoot->_cRows;     // _cCalcedRows excludes foot rows
        Assert (_cCalcedRows >=0);
    }

    //
    // Size the display nodes
    //

    if (   _aryColCalcs.Size() == 0  // set the size to 0, if there is no real content
        && _aryCaptions.Size() == 0) // (there are no real cells nor captions)
    {
        sizeTable.cy = psize->cy = 0;  // NETSCAPE: doesn't add the border or spacing or height if the table is empty.
    }
    SizeTableDispNode(ptci, *psize, sizeTable);

    if (ElementOwner()->IsAbsolute())
    {
        ElementOwner()->SendNotification(NTYPE_ELEMENT_SIZECHANGED);
    }

    //
    // Make sure we have a display node to render cellborder if we need one
    // (only collapsed borders, rules or frame)
    //

    if (psize->cx || psize->cy)
        EnsureTableBorderDispNode();

#ifdef PERFMETER
    if (!_fIncrementalRecalc && _fCalcedOnce )
    {
        MtAdd(Mt(CalculateLayout), +1, 0);
    }
#endif

    _fIncrementalRecalc = FALSE;

    _fCalcedOnce = TRUE;

    PerfLog(tagTableLayout, this, "-CalculateLayout");


#ifdef  TABLE_PERF
    ::StopCAP();
#endif
}

void
CTableLayout::CalcAbsolutePosCell(CTableCalcInfo *ptci, CTableCell *pCell)
{
    CSize              sizeCell;
    CTableCellLayout * pCellLayout;
    CTable           * pTable = ptci->Table();
    CTreeNode        * pNodeCell = pCell->GetFirstBranch();
    const CHeightUnitValue * puvHeight;

    puvHeight = (CHeightUnitValue *)&pNodeCell->GetFancyFormat()->_cuvHeight;

    pCellLayout = pCell->Layout();
    pCellLayout->_fContentsAffectSize = TRUE;
    sizeCell.cx = (int)pCellLayout->GetSpecifiedPixelWidth(ptci);
    if (sizeCell.cx <= 0)
        sizeCell.cx = (int)pCellLayout->_xMax;

    sizeCell.cy = 0;
    pCellLayout->CalcSizeAtUserWidth(ptci, &sizeCell);
    
    // if the height of the cell is specified, take it.
    if (puvHeight->IsSpecified())
    {
        int iPixelHeight = puvHeight->GetPixelHeight(ptci, pTable);
        if (iPixelHeight > sizeCell.cy)
        {
            sizeCell.cy = iPixelHeight;
            pCellLayout->SizeDispNode(ptci, sizeCell);
        }
    }


    pCellLayout->SetYProposed(0);
    pCellLayout->SetXProposed(0);

    return;
}

//+--------------------------------------------------------------------------------------
//
// Layout methods overriding CLayout
//
//---------------------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CalcSize, CTableLayout
//
//  Synopsis:   Calculate the size of the table
//
//--------------------------------------------------------------------------

DWORD
CTableLayout::CalcSize( CCalcInfo * pci,
                        SIZE *      psize,
                        SIZE *      psizeDefault)
{
    Assert(pci);
    Assert(psize);
    Assert(pci->_smMode != SIZEMODE_SET);
    Assert(ElementOwner());

    CScopeFlag      csfCalcing(this);
    DWORD           grfReturn = (pci->_grfLayout & LAYOUT_FORCE);
    CTable * pTable = Table();


    // Ignore requests on incomplete tables
#if DBG == 1
    if (!IsTagEnabled(tagTableRecalc))
#endif
    if (!CanRecalc() && !IsCalced())
    {
        psize->cx =
        psize->cy = 0;

        if (pci->_smMode == SIZEMODE_NATURAL)
            _fSizeThis = FALSE;

        TraceTagEx((tagCalcSize, TAG_NONAME,
                   "CalcSize  : Layout(0x%x, %S) size(%d,%d) mode(%S)",
                   this,
                   ElementOwner()->TagName(),
                   psize->cx,
                   psize->cy,
                   (    pci->_smMode == SIZEMODE_NATURAL
                        ? _T("NATURAL")
                    :   pci->_smMode == SIZEMODE_SET
                        ? _T("SET")
                    :   pci->_smMode == SIZEMODE_FULLSIZE
                        ? _T("FULLSIZE")
                    :   pci->_smMode == SIZEMODE_MMWIDTH
                        ? _T("MMWIDTH")
                        : _T("MINWIDTH"))));

        return grfReturn;
    }

    else if (TestLock(CElement::ELEMENTLOCK_BLOCKCALC) || !CanRecalc())
    {
        switch (pci->_smMode)
        {
        case SIZEMODE_NATURAL:
            _fSizeThis  = FALSE;

        case SIZEMODE_SET:
        case SIZEMODE_FULLSIZE:
            GetSize(psize);
            break;

        case SIZEMODE_MMWIDTH:
            psize->cx = _sizeMax.cx;
            psize->cy = _sizeMin.cx;
            break;

        case SIZEMODE_MINWIDTH:
            *psize = _sizeMin;
            break;
        }

        TraceTagEx((tagCalcSize, TAG_NONAME,
                   "CalcSize  : Layout(0x%x, %S) size(%d,%d) mode(%S)",
                   this,
                   ElementOwner()->TagName(),
                   psize->cx,
                   psize->cy,
                   (    pci->_smMode == SIZEMODE_NATURAL
                        ? _T("NATURAL")
                    :   pci->_smMode == SIZEMODE_SET
                        ? _T("SET")
                    :   pci->_smMode == SIZEMODE_FULLSIZE
                        ? _T("FULLSIZE")
                    :   pci->_smMode == SIZEMODE_MMWIDTH
                        ? _T("MMWIDTH")
                        : _T("MINWIDTH"))));

        return grfReturn;
    }

    TraceTag((tagTableCalc, "CTableLayout::CalcSize - Enter (0x%x), smMode = %x, grfLayout = %x", pTable, pci->_smMode, pci->_grfLayout));

    CTableCalcInfo  tci(pci, pTable, this);
    CSize           sizeOriginal = g_Zero.size;

    if (_fForceLayout)
    {
        tci._grfLayout |= LAYOUT_FORCE;
        _fForceLayout   = FALSE;
        grfReturn      |= LAYOUT_FORCE;
    }

    if (pci->_grfLayout & LAYOUT_FORCE)
    {
        _fAutoBelow   = FALSE;
        _fPositionSet = FALSE;
    }

    EnsureTableLayoutCache();

    _cNestedLevel = tci._cNestedCalcs;

    CWidthUnitValue     uvWidth  = GetFirstBranch()->GetCascadedwidth();
    CHeightUnitValue    uvHeight = GetFirstBranch()->GetCascadedheight();

    // For NS/IE compatibility, treat negative values as not present
    if (uvWidth.GetUnitValue() <= 0)
        uvWidth.SetNull();
    if (uvHeight.GetUnitValue() <= 0)
        uvHeight.SetNull();

    if (tci._smMode == SIZEMODE_NATURAL)
    {
        long    cxParent;
        BOOL    fWidthChanged;
        BOOL    fHeightChanged;

        GetSize(&sizeOriginal);

        //
        // Determine the appropriate parent width
        // (If width is a percentage, use the full parent size as the parent width;
        //  otherwise, use the available size as the parent width)
        //

        cxParent = (uvWidth.IsSpecified() && PercentWidth()
                            ? tci._sizeParent.cx
                            : psize->cx);

        //
        // Table width changes if
        //  a) Forced to re-examine
        //  b) Min/max values are dirty
        //  c) Width  is a percentage and parent width has changed
        //  d) Width  is not specified and the available space has changed
        //  d) Parent is smaller/equal to table minimum and table is greater than minimum
        //  e) Parent is greater/equal to table maximum and table is less than maximum
        //

        fWidthChanged =     (tci._grfLayout & LAYOUT_FORCE)
                        ||  _sizeMax.cx < 0
                        ||  (   (_fHavePercentageCol || _fHavePercentageInset || _fForceMinMaxOnResize || PercentWidth())
                            &&  cxParent != _sizeParent.cx)
                        ||  (   cxParent < _sizeParent.cx
                            &&  sizeOriginal.cx > _sizeMin.cx)
                        ||  (   cxParent > _sizeParent.cx
                            &&  sizeOriginal.cx < _sizeMax.cx);

        fHeightChanged = (PercentHeight() && tci._sizeParent.cy != _sizeParent.cy);

        //
        // Calculate a new size if
        //  a) The table is dirty
        //  b) Table width changed
        //  c) Height is a percentage and parent height has changed
        //  d) Not completed loading (table size is always dirty while loading)
        //

        _fSizeThis = _fSizeThis    ||
                     fWidthChanged ||
                     fHeightChanged||
                     !_fCompleted;

        //
        // If the table needs it, recalculate its size
        //

        if (_fSizeThis)
        {
            CSize   size;
            BOOL    fIncrementalRecalc = _fIncrementalRecalc;

            // Cache parent size
            _sizeParent.cx = cxParent;
            _sizeParent.cy = tci._sizeParent.cy;

            CalculateLayout(&tci, &size, fWidthChanged, fHeightChanged);

            *psize      = size;

            _fSizeThis  = FALSE;
            grfReturn  |= LAYOUT_THIS |
                          (size.cx != sizeOriginal.cx
                                ? LAYOUT_HRESIZE
                                : 0)  |
                          (size.cy != sizeOriginal.cy
                                ? LAYOUT_VRESIZE
                                : 0);

            if (fIncrementalRecalc)
            {
                 _dwTimeEndLastRecalc = GetTickCount();
                 _dwTimeBetweenRecalc += 1000;  // increase the interval between the incremental reaclcs by 1 sec.
            }

#if DBG == 1
            if (IsTagEnabled(tagTableDump))
            {
                DumpTable(_T("CalcSize(SIZEMODE_NATURAL)"));
            }
#endif
        }

        //
        // Otherwise, propagate the request through default handling
        //

        else
        {
            *psize = sizeOriginal;
        }
    }

    else if (  tci._smMode == SIZEMODE_MMWIDTH
            || tci._smMode == SIZEMODE_MINWIDTH
            )
    {

        if (_sizeMax.cx < 0 || (tci._grfLayout & LAYOUT_FORCE))
        {
            CElement::CLock   Lock(pTable, CElement::ELEMENTLOCK_SIZING);

            CalculateBorderAndSpacing(&tci);
            CalculateMinMax(&tci);

            _fSizeThis = TRUE;

            //
            // If an explicit width exists, then use that width (or the minimum
            // width of the table when the specified width is too narrow) for
            // both the minimum and maximum
            //

            if (    uvWidth.IsSpecified()
                &&  !uvWidth.IsSpecifiedInPercent())
            {
                long    cxWidth = uvWidth.GetPixelWidth(&tci, pTable);

                if (cxWidth < _sizeMin.cx)
                {
                    cxWidth = _sizeMin.cx;
                }
                else if (cxWidth > _sizeMin.cx)
                {
                    _sizeMin.cx = cxWidth;
                }

                if (cxWidth < _sizeMax.cx)
                {
                    _sizeMax.cx = cxWidth;
                }
            }
        }

        if (tci._smMode == SIZEMODE_MMWIDTH)
        {
            psize->cx = _sizeMax.cx;
            psize->cy = _sizeMin.cx;
        }
        else
        {
            *psize = _sizeMin;
        }
    }

    else
    {
        grfReturn = super::CalcSize(&tci, psize);
    }

    TraceTag((tagTableCalc, "CTableLayout::CalcSize - Exit (0x%x)", pTable));

    TraceTagEx((tagCalcSize, TAG_NONAME,
               "CalcSize  : Layout(0x%x, %S) size(%d,%d) mode(%S)",
               this,
               ElementOwner()->TagName(),
               psize->cx,
               psize->cy,
               (    tci._smMode == SIZEMODE_NATURAL
                    ? _T("NATURAL")
                :   tci._smMode == SIZEMODE_SET
                    ? _T("SET")
                :   tci._smMode == SIZEMODE_FULLSIZE
                    ? _T("FULLSIZE")
                :   tci._smMode == SIZEMODE_MMWIDTH
                    ? _T("MMWIDTH")
                    : _T("MINWIDTH"))));

    // If this table is nested, propagate out _fTableContainsCols, if set.
    if (pci->_fTableCalcInfo && tci._fTableContainsCols)
        ((CTableCalcInfo *) pci)->_fTableContainsCols = TRUE;


    if (pci->_smMode == SIZEMODE_NATURAL)
    {
        //
        //  Reset dirty state and remove posted layout request
        //

        _fDirty = FALSE;

        //
        // If any absolutely positioned sites need sizing, do so now
        //

        if (HasRequestQueue())
        {
            ProcessRequests(pci, sizeOriginal);
        }

        Reset(FALSE);
        Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
    }

    if (pci->_fTableCalcInfo && tci._fDontSaveHistory)
    {
        ((CTableCalcInfo *)pci)->_fDontSaveHistory = TRUE;  // propagate save history flag up.
    }

    return grfReturn;
}


//+------------------------------------------------------------------------
//
//  Member:     CTableLayout::GetSpecifiedPixelWidth
//
//  Synopsis:   get user width
//
//  Returns:    returns user's specified width of the table (0 if not set or
//              if specified in %%)
//              if user set's width <= 0 it will be ignored
//-------------------------------------------------------------------------

long
CTableLayout::GetSpecifiedPixelWidth(CTableCalcInfo * ptci, BOOL *pfSpecifiedInPercent)
{
    CWidthUnitValue uvWidth     = GetFirstBranch()->GetCascadedwidth();
    long            xTableWidth = 0;
    BOOL            fSpecifiedInPercent = FALSE;
    CTable      *   pTable = ptci->Table();

    // NS/IE compatibility, any value <= 0 is treated as <not present>
    if ( uvWidth.GetUnitValue() <= 0 )
    {
        uvWidth.SetNull();
    }

    if (uvWidth.IsSpecified())
    {
        if (uvWidth.IsSpecifiedInPixel())
        {
            xTableWidth = uvWidth.GetPixelWidth(ptci, pTable);
        }
        else
        {
            fSpecifiedInPercent = TRUE;
        }
    }

    if (pfSpecifiedInPercent)
    {
        *pfSpecifiedInPercent = fSpecifiedInPercent;
    }

    return xTableWidth;
}
