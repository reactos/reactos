//+--------------------------------------------------------------------------
//
//  File:       shape.cxx
//
//  Contents:   CShape - generic shape class implementation
//
//  Classes:    CShape
//
//---------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_DRAWINFO_HXX_
#include "drawinfo.hxx"
#define X_DRAWINFO_HXX_
#endif

DeclareTag(tagShape, "CShape", "CShape methods");
MtDefine(CShape, Utilities, "CShape")
MtDefine(CWigglyAry, Utilities, "CWigglyAry")
MtDefine(CWigglyAry_pv, Utilities, "CWigglyAry_pv")


//+-------------------------------------------------------------------------
//
//  Method:     CShape::DrawFocus
//
//  Synopsis:   Draw the boundary of the region(s) enclosed by this shape to
//              indicate that this shape has the focus.
//
//--------------------------------------------------------------------------

void
CShape::DrawShape(CFormDrawInfo * pDI)
{
    POINT           pt;
    HDC             hDC = pDI->GetDC() ;
    COLORREF        crOldBk, crOldFg ;
    CRect           rectBound;
    const SIZECOORD cThick = (_cThick) ? _cThick : ((g_fHighContrastMode) ? 2 : 1);

    GetBoundingRect(&rectBound);
    if (!rectBound.Intersects(*pDI->ClipRect()))
        return;

    crOldBk = SetBkColor (hDC, RGB (0,0,0)) ;
    crOldFg = SetTextColor (hDC, RGB(0xff,0xff,0xff)) ;

    GetViewportOrgEx (hDC, &pt) ;
    SetBrushOrgEx(hDC, POSITIVE_MOD(pt.x,8)+POSITIVE_MOD(rectBound.left, 8),
                       POSITIVE_MOD(pt.y,8)+POSITIVE_MOD(rectBound.top, 8), NULL);

    Draw(hDC, cThick);

    SetTextColor (hDC, crOldFg);
    SetBkColor   (hDC, crOldBk);
}



//+-------------------------------------------------------------------------
//
//  Method:     CRectShape::GetBoundingRect
//
//  Synopsis:   Return the bounding rectangle of the region(s) enclosed by
//              this shape.
//
//--------------------------------------------------------------------------
void
CRectShape::GetBoundingRect(CRect * pRect)
{
    Assert(pRect);
    *pRect = _rect;
}

//+-------------------------------------------------------------------------
//
//  Method:     CRectShape::OffsetShape
//
//  Synopsis:   Shifts the shape by the given amounts along x and y axes.
//
//--------------------------------------------------------------------------
void
CRectShape::OffsetShape(const CSize & sizeOffset)
{
    _rect.OffsetRect(sizeOffset);
}

//+-------------------------------------------------------------------------
//
//  Method:     CRectShape::Draw
//
//  Synopsis:   Draw the boundary of the region(s) enclosed by this shape
//
//--------------------------------------------------------------------------

void
CRectShape::Draw(HDC hDC, SIZECOORD cThick)
{
    static short bBrushBits[8] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};

    HBITMAP         hbm;
    HBRUSH          hBrushOld;

    TraceTag((tagShape, "CRectShape::Draw"));

    hbm = CreateBitmap (8, 8, 1, 1, (LPBYTE)bBrushBits) ;
    hBrushOld = (HBRUSH)SelectObject(hDC, CreatePatternBrush (hbm));

    PatBltRect(hDC, &_rect, cThick, PATINVERT);

    DeleteObject(SelectObject(hDC, hBrushOld));
    DeleteObject (hbm);
}



//+-------------------------------------------------------------------------
//
//  Method:     CCircleShape::Draw
//
//  Synopsis:   Draw the boundary of the region(s) enclosed by this shape
//
//--------------------------------------------------------------------------

void
CCircleShape::Draw(HDC hDC, SIZECOORD cThick)
{
    HBRUSH  hBrushOld;
    HPEN    hPenOld;
    int     nROPOld;

    TraceTag((tagShape, "CCircleShape::Draw"));

    // Select transparent brush to fill the circle
    nROPOld = SetROP2(hDC, R2_XORPEN);
    hPenOld = (HPEN)SelectObject(hDC, (HBRUSH)GetStockObject(WHITE_PEN));
    hBrushOld  = (HBRUSH)SelectObject(hDC, (HBRUSH)GetStockObject(HOLLOW_BRUSH));


    // BUGBUG (MohanB) cThick ignored
    Ellipse(hDC, _rect.left, _rect.top, _rect.right, _rect.bottom);

    SelectObject(hDC, hBrushOld);
    SelectObject(hDC, hPenOld);
    SetROP2(hDC, nROPOld);
}

//+-------------------------------------------------------------------------
//
//  Method:     CCircleShape::Set
//
//  Synopsis:   Set using center and radius
//
//--------------------------------------------------------------------------
void
CCircleShape::Set(POINTCOORD xCenter, POINTCOORD yCenter, SIZECOORD radius)
{
    _rect.SetRect(xCenter - radius, yCenter - radius, xCenter + radius, yCenter + radius);
}

//+-------------------------------------------------------------------------
//
//  Method:     GetPolyBoundingRect
//
//  Synopsis:   Return the bounding rectangle of the given polygon.
//
//--------------------------------------------------------------------------
void
GetPolyBoundingRect(CPointAry& aryPoint, CRect * pRect)
{
    UINT    c;
    POINT * pPt;

    Assert(pRect);
    pRect->SetRectEmpty();

    for (c = aryPoint.Size(), pPt = aryPoint; c > 0; c--, pPt++)
    {
        pRect->Union(*pPt);
    }

}

//+-------------------------------------------------------------------------
//
//  Method:     CPolyShape::GetBoundingRect
//
//  Synopsis:   Return the bounding rectangle of the region(s) enclosed by
//              this shape.
//
//--------------------------------------------------------------------------
void
CPolyShape::GetBoundingRect(CRect * pRect)
{
    GetPolyBoundingRect(_aryPoint, pRect);
}

//+-------------------------------------------------------------------------
//
//  Method:     CPolyShape::OffsetShape
//
//  Synopsis:   Shifts the shape by the given amounts along x and y axes.
//
//--------------------------------------------------------------------------
void
CPolyShape::OffsetShape(const CSize & sizeOffset)
{
    CPoint *    ppt;
    long        cPoints;

    for(cPoints = _aryPoint.Size(), ppt = (CPoint *)&(_aryPoint[0]);
        cPoints > 0;
        cPoints--, ppt++)
    {
        *ppt += sizeOffset;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     DrawPoly
//
//  Synopsis:   Draw the boundary of the given polygon.
//
//--------------------------------------------------------------------------

void
DrawPoly(CPointAry & aryPoint, HDC hDC, SIZECOORD cThick)
{
    POINT * ppt;
    UINT    c;
    HPEN    hPenOld;
    HBRUSH  hBrushOld;
    int     nROPOld;

    // Do we have enough points to draw a polygon ?
    if (aryPoint.Size() < 2)
        return;

    // BUGBUG (MohanB) cThick ignored

    nROPOld = SetROP2(hDC, R2_XORPEN);
    hPenOld = (HPEN)SelectObject(hDC, (HBRUSH)GetStockObject(WHITE_PEN));
    hBrushOld  = (HBRUSH)SelectObject(hDC, (HBRUSH)GetStockObject(HOLLOW_BRUSH));

    MoveToEx(hDC, aryPoint[0].x, aryPoint[0].y, (POINT *)NULL);
    for(c = aryPoint.Size(), ppt = &(aryPoint[1]);
        c > 1;                  // c > 1, because we MoveTo'd the first pt
        ppt++, c--)
    {
        LineTo(hDC, ppt->x, ppt->y);
    }
    //
    // If there are only 2 points in the polygon, we don't want to draw
    // the same line twice and end up with nothing!
    //

    if(aryPoint.Size() != 2)
    {
        LineTo(hDC, aryPoint[0].x, aryPoint[0].y);

    }
    SelectObject(hDC, hBrushOld);
    SelectObject(hDC, hPenOld);
    SetROP2(hDC, nROPOld);
}


//+-------------------------------------------------------------------------
//
//  Method:     CPolyShape::Draw
//
//  Synopsis:   Draw the boundary of the region(s) enclosed by this shape
//
//--------------------------------------------------------------------------

void
CPolyShape::Draw(HDC hDC, SIZECOORD cThick)
{
    TraceTag((tagShape, "CPolyShape::Draw"));

    DrawPoly(_aryPoint, hDC, cThick);

}


//+-------------------------------------------------------------------------
//
//  Method:     CWigglyShape::~CWigglyShape
//
//  Synopsis:   Release the CRectShape objects in the array
//
//--------------------------------------------------------------------------
CWigglyShape::~CWigglyShape()
{
    UINT           c;
    CRectShape **  ppWiggly;

    for(c = _aryWiggly.Size(), ppWiggly = _aryWiggly; c > 0; c--, ppWiggly++)
    {
        delete *ppWiggly;
        *ppWiggly = NULL;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CWigglyShape::GetBoundingRect
//
//  Synopsis:   Return the bounding rectangle of the region(s) enclosed by
//              this shape.
//
//--------------------------------------------------------------------------
void
CWigglyShape::GetBoundingRect(CRect * pRect)
{
    UINT           c;
    CRectShape **  ppWiggly;
    CRect          rectWiggly;

    Assert(pRect);
    pRect->SetRectEmpty();

    for(c = _aryWiggly.Size(), ppWiggly = _aryWiggly; c > 0; c--, ppWiggly++)
    {
        (*ppWiggly)->GetBoundingRect(&rectWiggly);
        pRect->Union(rectWiggly);
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CWigglyShape::OffsetShape
//
//  Synopsis:   Shifts the shape by the given amounts along x and y axes.
//
//--------------------------------------------------------------------------
void
CWigglyShape::OffsetShape(const CSize & sizeOffset)
{
    CRectShape ** ppWiggly;
    long          cWigglies;

    for(cWigglies = _aryWiggly.Size(), ppWiggly = _aryWiggly;
        cWigglies > 0;
        cWigglies--, ppWiggly++)
    {
        (*ppWiggly)->_rect.OffsetRect(sizeOffset);

    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CWigglyShape::Draw
//
//  Synopsis:   Draw the boundary of the region(s) enclosed by this shape
//              Adjustments are made for bottom and right when they have
//              neighboring rects to make sure the boundaries that adjoin
//              draw on top of each other to XOR each other out of sight.
//              This gives the appearance of a singular containging box.
//
//--------------------------------------------------------------------------

void
CWigglyShape::Draw(HDC hDC, SIZECOORD cThick)
{
    static short bBrushBits[8] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};

    HBITMAP         hbm;
    HBRUSH          hBrushOld;
    int             c;
    CRectShape *    pWiggly;

    TraceTag((tagShape, "CWigglyShape::Draw"));

    hbm = CreateBitmap (8, 8, 1, 1, (LPBYTE)bBrushBits) ;
    hBrushOld = (HBRUSH)SelectObject(hDC, CreatePatternBrush (hbm));


    for(c = 0; c < _aryWiggly.Size(); c++)
    {
        pWiggly = _aryWiggly[c];
        // In the case there is just one rect, just draw the rect
        // This will be the majority of cases.
        if(_aryWiggly.Size() == 1)
        {
            pWiggly->Draw(hDC, cThick);
        }
        else
        {
            // We have more than one rect. Most likely they will share
            // at least part of one adjoining side. We will clip this out
            CRect rcCurrent;
            CRectShape * pNeighborWiggly;
            CRect rcNeighbor;
            int cNeighbor;
            
            pWiggly->GetBoundingRect(&rcCurrent);
 
            // 1. Draw the top line
            PatBlt (hDC,
                    rcCurrent.left,
                    rcCurrent.top,
                    rcCurrent.right - rcCurrent.left,
                    cThick,
                    PATINVERT);

            // 2. Draw the bottom line. Look to see if we have an adjoining rect on the bottom
            // If we do, don't subtract thickness so we XOR our neighbor's top line
            BOOL fBottomNeighbor = FALSE;
            
            // We are not guaranteed that rects are ordered in the array
            // by position. Look through the entire array until match is made.
            for(cNeighbor = 0; cNeighbor < _aryWiggly.Size(); cNeighbor++)
            {
                pNeighborWiggly = _aryWiggly[cNeighbor];
                pNeighborWiggly->GetBoundingRect(&rcNeighbor);

                if(rcNeighbor.top == rcCurrent.bottom &&
                   (rcCurrent.left < rcNeighbor.right && 
                    rcCurrent.right > rcNeighbor.left))
                {
                    fBottomNeighbor = TRUE;
                    break;
                }
            }
            if(!fBottomNeighbor)
            {
                PatBlt (hDC,
                        rcCurrent.left,
                        rcCurrent.bottom - cThick,
                        rcCurrent.right - rcCurrent.left,
                        cThick,
                        PATINVERT);
            }
            else
            {
                PatBlt (hDC,
                        rcCurrent.left,
                        rcCurrent.bottom,
                        rcCurrent.right - rcCurrent.left,
                        cThick,
                        PATINVERT);
            }
            
            // 3. Draw the left side
            PatBlt (hDC,
                    rcCurrent.left,
                    rcCurrent.top,
                    cThick,
                    rcCurrent.bottom - rcCurrent.top,
                    PATINVERT);
          

            // 4. Draw the right side. Look to see if we have an adjoining rect on the right
            // If we do, don't subtract thickness so we XOR our neighbor's line
            BOOL fRightNeighbor = FALSE;
            
            // We are not guaranteed that rects are ordered in the array
            // by position. Look through the entire array until match is made.
            for(cNeighbor = 0; cNeighbor < _aryWiggly.Size(); cNeighbor++)
            {
                pNeighborWiggly = _aryWiggly[cNeighbor];
                pNeighborWiggly->GetBoundingRect(&rcNeighbor);

                if(rcNeighbor.left == rcCurrent.right &&
                   (rcCurrent.top < rcNeighbor.bottom && 
                    rcCurrent.bottom > rcNeighbor.top))
                {
                    fRightNeighbor = TRUE;
                    break;
                }
            }
               
            if(fRightNeighbor)
            {
                PatBlt (hDC,
                        rcCurrent.right,
                        rcCurrent.top,
                        cThick,
                        rcCurrent.bottom - rcCurrent.top,
                        PATINVERT);
            }
            else
            {
                PatBlt (hDC,
                        rcCurrent.right - cThick,
                        rcCurrent.top,
                        cThick,
                        rcCurrent.bottom - rcCurrent.top,
                        PATINVERT);
            }
        }
    }

    DeleteObject(SelectObject(hDC, hBrushOld));
    DeleteObject (hbm);

}


