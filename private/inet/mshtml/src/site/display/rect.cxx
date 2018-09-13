//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       rect.cxx
//
//  Contents:   Class to make rectangles easier to deal with.
//
//  Classes:    CRect
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CRect::Union
//              
//  Synopsis:   Extend rect to contain the given point.  If the rect is
//              initially empty, it will contain only the point afterwards.
//              
//  Arguments:  p       point to extend to
//              
//----------------------------------------------------------------------------

void
CRect::Union(const POINT& p)
{
    if (IsRectEmpty())
        SetRect(p.x,p.y,p.x+1,p.y+1);
    else
    {
        if (p.x < left) left = p.x;
        if (p.y < top) top = p.y;
        if (p.x >= right) right = p.x+1;
        if (p.y >= bottom) bottom = p.y+1;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CRect::Intersects
//              
//  Synopsis:   Determine whether the given rect intersects this rect without
//              taking the time to compute the intersection.
//              
//  Arguments:  rc      other rect
//              
//  Returns:    TRUE if the rects are not empty and overlap    
//              
//----------------------------------------------------------------------------

BOOL
CRect::Intersects(const RECT& rc) const
{
    return
        left < rc.right &&
        top < rc.bottom &&
        right > rc.left &&
        bottom > rc.top &&
        !IsRectEmpty() &&
        !((CRect&)rc).IsRectEmpty();
}


//+---------------------------------------------------------------------------
//
//  Function:   CalcScrollDelta
//
//  Synopsis:   Calculates the distance needed to scroll to make a given rect
//              visible inside this rect.
//
//  Arguments:  rc          Rectangle which needs to be visible inside this rect
//              psizeScroll Amount to scroll
//              vp, hp      Where to "pin" given RECT inside this RECT
//
//  Returns:    TRUE if scrolling required.
//
//----------------------------------------------------------------------------

BOOL
CRect::CalcScrollDelta(
        const CRect& rc,
        CSize* psizeScroll,
        CRect::SCROLLPIN spVert,
        CRect::SCROLLPIN spHorz) const
{
    int         i;
    long        cxLeft;
    long        cxRight;
    SCROLLPIN   sp;

    Assert(psizeScroll);

    if (spVert == SP_MINIMAL && spHorz == SP_MINIMAL && Contains(rc))
    {
        *psizeScroll = g_Zero.size;
        return FALSE;
    }

    sp = spHorz;
    for (i = 0; i < 2; i++)
    {
        // Calculate amount necessary to "pin" the left edge
        cxLeft = rc[i] - (*this)[i];

        // Examine right edge only if not "pin"ing to the left
        if (sp != SP_TOPLEFT)
        {
            Assert(sp == SP_BOTTOMRIGHT || sp == SP_MINIMAL);

            cxRight = (*this)[i+2] - rc[i+2];

            // "Pin" the inner RECT to the right side of the outer RECT
            if (sp == SP_BOTTOMRIGHT)
            {
                cxLeft = -cxRight;
            }

            // Otherwise, move the minimal amount necessary to make the
            // inner RECT visible within the outer RECT
            // (This code will try to make the entire inner RECT visible
            //  and gives preference to the left edge)
            else if (cxLeft > 0)
            {
                if (cxRight >= 0)
                {
                    cxLeft = 0;
                }
                else if (-cxRight < cxLeft)
                {
                    cxLeft = -cxRight;
                }
            }
        }
        (*psizeScroll)[i] = cxLeft;
        sp = spVert;
    }

    return !psizeScroll->IsZero();
}


//+---------------------------------------------------------------------------
//
//  Member:     CRect::CountContainedCorners
//              
//  Synopsis:   Count how many corners of the given rect are contained by
//              this rect.  This is tricky, because a rect doesn't technically
//              contain any of its corners except the top left.  This method
//              returns a count of 4 for rc.CountContainedCorners(rc).
//              
//  Arguments:  rc      rect to count contained corners for
//              
//  Returns:    -1 if rectangles do not intersect, or 0-4 if they do.  Zero
//              if rc completely contains this rect.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

int
CRect::CountContainedCorners(const RECT& rc) const
{
    if (!Intersects(rc))
        return -1;
    
    int c = 0;
    if (rc.left >= left) {
        if (rc.top >= top) c++;
        if (rc.bottom <= bottom) c++;
    }
    if (rc.right <= right) {
        if (rc.top >= top) c++;
        if (rc.bottom <= bottom) c++;
    }
    return c;
}




