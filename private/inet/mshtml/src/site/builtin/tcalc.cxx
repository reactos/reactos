//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       tcalc.cxx
//
//  Contents:   CTable and related classes.
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include <download.hxx>
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include <_disp.h>
#endif

#ifdef  TABLE_PERF
extern "C" void _stdcall StartCAP();
extern "C" void _stdcall StopCAP();
#endif


//+-------------------------------------------------------------------------
//
//  Method:     AdjustForCell
//
//  Synopsis:   Promote the cell width into the column (if certain
//              criteria are met)
//
//  Arguments:  pTable  - Table pointer
//              iPixels - user specified pixel width of the cell (0 if none)
//              puvWidth- user specifed Unit Value
//                             BOOL                   fUnsizedColumn,
//                             BOOL                   fFirstRow,
//                             CCalcInfo *            pci,
//                             int                    xMin,
//                             int                    xMax
//
//--------------------------------------------------------------------------

void
CTableColCalc::AdjustForCell(CTableLayout *         pTableLayout,
                             int                    iPixels,
                             const CWidthUnitValue *puvWidth,
                             BOOL                   fUnsizedColumn,
                             BOOL                   fFirstRow,
                             CCalcInfo *            pci,
                             int                    xMin,
                             int                    xMax)
{

    // cache width attribute in column (if appropriate)
    Assert (pTableLayout);

    if ((fUnsizedColumn || fFirstRow) && puvWidth->IsSpecified())
    {
        // set column unit width if smaller then the cell width
        if (!IsWidthSpecified())
        {
            if(!_fDontSetWidthFromCell)
            {
                // NETSCAPE: DON'T propagate the PIXEL width of the cell to the column if there are more then
                // 1 cell in that column
                if (puvWidth->IsSpecifiedInPixel())
                {
                    if (iPixels)    // bugfix: if width is set through style width:0px, then it will be ignored.
                    {
                        _uvWidth = *puvWidth;
                        SetPixelWidth(pci, iPixels);
                        _fWidthFromCell = TRUE;
                    }
                }
                else
                {
                    _uvWidth = *puvWidth;
                    _fWidthFromCell = TRUE;
                }
            }
        }
        else if (puvWidth->IsSpecifiedInPercent())
        {
            if (IsWidthSpecifiedInPercent())
            {
                // set it if smaller
                if (GetWidthUnitValue() < puvWidth->GetUnitValue())
                {
                    _uvWidth = *puvWidth;
                }
            }
            else
            {
                // percent has precedence over normal width
                _uvWidth = *puvWidth;
            }
        }
        else if (!IsWidthSpecifiedInPercent())
        {
            // set if smaller
            if (GetPixelWidth(pci, pTableLayout->ElementOwner()) < iPixels)
            {
                _uvWidth = *puvWidth;
                SetPixelWidth(pci, iPixels);
            }
        }
    }

    // adjust column width

    if (fUnsizedColumn || fFirstRow || pTableLayout->_fAlwaysMinMaxCells)
    {
        if (_xMax < xMax)
        {
            _xMax = xMax;

            //NETSCAPE: if the new MAX is greater and the column width was set from the cell,
            //          don't propagate the user's width to the column.
            if (_fWidthFromCell && IsWidthSpecifiedInPixel() && !puvWidth->IsSpecified())
            {
                // reset the column uvWidth
                _fDontSetWidthFromCell = TRUE;
                ResetWidth();
            }
            if (!fUnsizedColumn && !puvWidth->IsSpecifiedInPercent())
            {
                _xMin   =
                _xWidth = _xMax;
            }
        }
        if (_xMin < xMin)
        {
            _xMin = xMin;
        }

        // this will adjust col max to user setting
        AdjustMaxToUserWidth(pci, pTableLayout);

        // use col max
        // xMax = _xMax;
    }

    _fAdjustedForCell = TRUE;
    return;
}


//+-------------------------------------------------------------------------
//
//  Method:     AdjustForCol
//
//  Synopsis:   Promote the cell width into the column (if certain
//              criteria are met)
//
//  Arguments:  pTable  - Table pointer
//              iPixels - user specified pixel width of the cell (0 if none)
//              pci     - CalcInfo,
//
//--------------------------------------------------------------------------

void
CTableColCalc::AdjustForCol(const CWidthUnitValue *puvWidth,
                            int                    iColWidth,
                            CCalcInfo             *pci,
                            int                    cColumns)
{
    _uvWidth = *puvWidth;

    if (IsWidthSpecifiedInPixel())
    {
        SetPixelWidth(pci, iColWidth);
        _xMin = _xMax = _xWidth = iColWidth;
    }
    else
    {
        if (cColumns != 1)
        {
            int ip = GetPercentWidth();
            SetPercentWidth(ip/cColumns);
        }
        _xMin = 1;
        _xMax = pci->_sizeParent.cx;
        _xWidth = iColWidth;
    }

    return;
}
