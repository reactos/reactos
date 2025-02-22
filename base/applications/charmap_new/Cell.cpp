/*
* PROJECT:     ReactOS Character Map
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        base/applications/charmap/cell.cpp
* PURPOSE:     Class for each individual cell
* COPYRIGHT:   Copyright 2015 Ged Murphy <gedmurphy@reactos.org>
*/


#include "precomp.h"
#include "Cell.h"


/* DATA *****************************************************/


/* PUBLIC METHODS **********************************************/

CCell::CCell(
    _In_ HWND hParent
    ) :
    CCell(hParent, RECT{0})
{
}

CCell::CCell(
    _In_ HWND hParent,
    _In_ RECT& CellCoordinates
    ) :
    m_hParent(hParent),
    m_CellCoordinates(CellCoordinates),
    m_Char(L'*'),
    m_bHasFocus(false),
    m_bIsLarge(false)
{
}

CCell::~CCell()
{
}

bool
CCell::OnPaint(_In_ PAINTSTRUCT &PaintStruct)
{
    // Check if this cell is in our paint region
    BOOL NeedsPaint; RECT rect;
    NeedsPaint = IntersectRect(&rect,
                               &PaintStruct.rcPaint,
                               &m_CellCoordinates);
    if (NeedsPaint == FALSE)
        return false;



    // Draw the cell border
    BOOL b = Rectangle(PaintStruct.hdc,
                       m_CellCoordinates.left,
                       m_CellCoordinates.top,
                       m_CellCoordinates.right,
                       m_CellCoordinates.bottom);

    // Calculate an internal drawing canvas for the cell
    RECT Internal;
    CopyRect(&Internal, &m_CellCoordinates);
    InflateRect(&Internal, -1, -1);

    // Check if this cell has focus
    if (m_bHasFocus)
    {
        // Draw the smaller cell to make it look selected
        Rectangle(PaintStruct.hdc,
                  Internal.left,
                  Internal.top,
                  Internal.right,
                  Internal.bottom);
    }

    int Success;
    Success = DrawTextW(PaintStruct.hdc,
                        &m_Char,
                        1,
                        &Internal,
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    return (Success != 0);
}

void
CCell::SetCellCoordinates(
    _In_ RECT& Coordinates
    )
{
    m_CellCoordinates = Coordinates;
}
