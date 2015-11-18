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
    ch(L'*'),
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

    // Check if this cell has focus
    if (m_bHasFocus)
    {
        // Take a copy of the border dims and make it slightly smaller
        RECT Internal;
        CopyRect(&Internal, &m_CellCoordinates);
        InflateRect(&Internal, -1, -1);

        // Draw the smaller cell to make it look selected
        Rectangle(PaintStruct.hdc,
                  Internal.left,
                  Internal.top,
                  Internal.right,
                  Internal.bottom);
    }

    return true;
}

void
CCell::SetCellCoordinates(
    _In_ RECT& Coordinates
    )
{
    m_CellCoordinates = Coordinates;
}