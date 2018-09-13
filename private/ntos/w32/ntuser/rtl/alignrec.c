/**************************************************************************\
* Module Name: mergerec.c
*
* Contains all the code to reposition rectangles
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* NOTES:
*
* History:
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define MONITORS_MAX 10

#define RectCenterX(prc)    ((prc)->left+((prc)->right-(prc)->left)/2)
#define RectCenterY(prc)    ((prc)->top+((prc)->bottom-(prc)->top)/2)

// ----------------------------------------------------------------------------
//
//  INTERSECTION_AXIS()
//      This macro tells us how a particular set of overlapping rectangles
//  should be adjusted to remove the overlap.  It is basically a condensed
//  version of a lookup table that does the same job.  The parameters for the
//  macro are two rectangles, where one is the intersection of the other with
//  a third (unspecified) rectangle.  The macro compares the edges of the
//  rectangles to determine which sides of the intersection were "caused" by
//  the source rectangle.  In the pre-condensed version of this macro, the
//  results of these comparisons (4 bits) would be used to index into a 16
//  entry table which specifies the way to resolve the overlap.  However, this
//  is highly redundant, as the table would actually represents several rotated
//  and/or inverted instances of a few basic relationships:
//
//  Horizontal Vertical  Diagonal  Contained       Crossing
//      *--*    *-----*   *---*     *-----*         *----*
//   *--+* |    | *-* |   | *-+-*   | *-* |       *-+----+-*
//   |  || |    *-+-+-*   | | | |   | | | |  and  | |    | |
//   *--+* |      | |     *-+-* |   | *-* |       *-+----+-*
//      *--*      *-*       *---*   *-----*         *----*
//
//  What we are really interested in determining is whether we "should" move
//  the rectangles horizontally or vertically to resolve the overlap, hence we
//  are testing for three states: Horizontal, Vertical and Don't Know.
//
//  The macro gives us these three states by XORing the high and low bits of
//  of the comparison to reduce the table to 4 cases where 1 and 2 are
//  vertical and horizontal respectively, and then subtracting 1 so that the
//  2 bit signifies "unknown-ness."
//
//  Note that there are some one-off cases in the comparisons because we are
//  not actually looking at the third rectangle.  However this greatly reduces
//  the complexity so these small errors are acceptible given the scale of the
//  rectangles we are comparing.
//
// ----------------------------------------------------------------------------
#define INTERSECTION_AXIS(a, b) \
    (((((a->left == b->left) << 1) | (a->top == b->top)) ^ \
    (((a->right == b->right) << 1) | (a->bottom == b->bottom))) - 1)

#define INTERSECTION_AXIS_VERTICAL      (0)
#define INTERSECTION_AXIS_HORIZONTAL    (1)
#define INTERSECTION_AXIS_UNKNOWN(code) (code & 2)

// ----------------------------------------------------------------------------
//
//  CenterRectangles()
//      Move all the rectangles so their origin is the center of their union.
//
// ----------------------------------------------------------------------------
void NEAR PASCAL CenterRectangles(LPRECT arc, UINT count)
{
    LPRECT lprc, lprcL;
    RECT rcUnion;

    CopyRect(&rcUnion, arc);

    lprcL = arc + count;
    for (lprc = arc + 1; lprc < lprcL; lprc++)
    {
        UnionRect(&rcUnion, &rcUnion, lprc);
    }

    for (lprc = arc; count; count--)
    {
        OffsetRect(lprc, -RectCenterX(&rcUnion), -RectCenterY(&rcUnion));
        lprc++;
    }
}

// ----------------------------------------------------------------------------
//
//  RemoveOverlap()
//    This is called from RemoveOverlaps to resolve conflicts when two
//  rectangles overlap.  It returns the PMONITOR for the monitor it decided to
//  move.  This routine always moves rectangles away from the origin so it can
//  be used to converge on a zero-overlap configuration.
//
//  This function will bias slightly toward moving lprc2 (all other things
//  being equal).
//
// ----------------------------------------------------------------------------
LPRECT NEAR PASCAL RemoveOverlap(LPRECT lprc1, LPRECT lprc2, LPRECT lprcI)
{
    LPRECT lprcMove, lprcStay;
    POINT ptC1, ptC2;
    BOOL fNegative;
    BOOL fC1Neg;
    BOOL fC2Neg;
    int dC1, dC2;
    int xOffset;
    int yOffset;
    int nAxis;

    //
    // Compute the centers of both rectangles.  We will need them later.
    //
    ptC1.x = RectCenterX(lprc1);
    ptC1.y = RectCenterY(lprc1);
    ptC2.x = RectCenterX(lprc2);
    ptC2.y = RectCenterY(lprc2);

    //
    // Decide whether we should move things horizontally or vertically.  All
    // this goop is here so it will "feel" right when the system needs to
    // move a monitor on you.
    //
    nAxis = INTERSECTION_AXIS(lprcI, lprc1);

    if (INTERSECTION_AXIS_UNKNOWN(nAxis))
    {
        //
        // Is this a "big" intersection between the two rectangles?
        //
        if (PtInRect(lprcI, ptC1) || PtInRect(lprcI, ptC2))
        {
            //
            // This is a "big" overlap.  Decide if the rectangles
            // are aligned more "horizontal-ish" or "vertical-ish."
            //
            xOffset = ptC1.x - ptC2.x;
            if (xOffset < 0)
                xOffset *= -1;
            yOffset = ptC1.y - ptC2.y;
            if (yOffset < 0)
                yOffset *= -1;

            if (xOffset >= yOffset)
                nAxis = INTERSECTION_AXIS_HORIZONTAL;
            else
                nAxis = INTERSECTION_AXIS_VERTICAL;
        }
        else
        {
            //
            // This is a "small" overlap.  Move the rectangles the
            // smallest distance that will fix the overlap.
            //
            if ((lprcI->right - lprcI->left) <= (lprcI->bottom - lprcI->top))
                nAxis = INTERSECTION_AXIS_HORIZONTAL;
            else
                nAxis = INTERSECTION_AXIS_VERTICAL;
        }
    }

    //
    // We now need to pick the rectangle to move.  Move the one
    // that is further from the origin along the axis of motion.
    //
    if (nAxis == INTERSECTION_AXIS_HORIZONTAL)
    {
        dC1 = ptC1.x;
        dC2 = ptC2.x;
    }
    else
    {
        dC1 = ptC1.y;
        dC2 = ptC2.y;
    }

    if ((fC1Neg = (dC1 < 0)) != 0)
        dC1 *= -1;

    if ((fC2Neg = (dC2 < 0)) != 0)
        dC2 *= -1;

    if (dC2 < dC1)
    {
        lprcMove     = lprc1;
        lprcStay     = lprc2;
        fNegative    = fC1Neg;
    }
    else
    {
        lprcMove     = lprc2;
        lprcStay     = lprc1;
        fNegative    = fC2Neg;
    }

    //
    // Compute a new home for the rectangle and put it there.
    //
    if (nAxis == INTERSECTION_AXIS_HORIZONTAL)
    {
        int xPos;

        if (fNegative)
            xPos = lprcStay->left - (lprcMove->right - lprcMove->left);
        else
            xPos = lprcStay->right;

        xOffset = xPos - lprcMove->left;
        yOffset = 0;
    }
    else
    {
        int yPos;

        if (fNegative)
            yPos = lprcStay->top - (lprcMove->bottom - lprcMove->top);
        else
            yPos = lprcStay->bottom;

        yOffset = yPos - lprcMove->top;
        xOffset = 0;
    }

    OffsetRect(lprcMove, xOffset, yOffset);
    return lprcMove;
}

// ----------------------------------------------------------------------------
//
//  RemoveOverlaps()
//    This is called from CleanupDesktopRectangles make sure the monitor array
//  is non-overlapping.
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RemoveOverlaps(LPRECT arc, UINT count)
{
    LPRECT lprc1, lprc2, lprcL;

    //
    // Center the rectangles around a common origin.  We will move them outward
    // when there are conflicts so centering (a) reduces running time and
    // hence (b) reduces the chances of totally mangling the positions.
    //
    CenterRectangles(arc, count);

    //
    // Now loop through the array fixing any overlaps.
    //
    lprcL = arc + count;
    lprc2 = arc + 1;

ReScan:
    while (lprc2 < lprcL)
    {
        //
        // Scan all rectangles before this one looking for intersections.
        //
        for (lprc1 = arc; lprc1 < lprc2; lprc1++)
        {
            RECT rcI;

            //
            // Move one of the rectanges if there is an intersection.
            //
            if (IntersectRect(&rcI, lprc1, lprc2))
            {
                //
                // Move one of the rectangles out of the way and then restart
                // the scan for overlaps with that rectangle (since moving it
                // may have created new overlaps).
                //
                lprc2 = RemoveOverlap(lprc1, lprc2, &rcI);
                goto ReScan;
            }
        }

        lprc2++;
    }
}

// ----------------------------------------------------------------------------
//
//  AddNextContiguousRectangle()
//    This is called from RemoveGaps to find the next contiguous rectangle
//  in the array.  If there are no more contiguous rectangles it picks the
//  closest rectangle and moves it so it is contiguous.
//
// ----------------------------------------------------------------------------
LPRECT FAR * NEAR PASCAL AddNextContiguousRectangle(LPRECT FAR *aprc,
    LPRECT FAR *pprcSplit, UINT count)
{
    LPRECT FAR *pprcL;
    LPRECT FAR *pprcTest;
    LPRECT FAR *pprcAxis;
    LPRECT FAR *pprcDiag;
    UINT dAxis = (UINT)-1;
    UINT dDiag = (UINT)-1;
    POINT dpAxis;
    POINT dpDiag;
    POINT dpMove;

    pprcL = aprc + count;

    for (pprcTest = aprc; pprcTest < pprcSplit; pprcTest++)
    {
        LPRECT lprcTest = *pprcTest;
        LPRECT FAR *pprcScan;

        for (pprcScan = pprcSplit; pprcScan < pprcL; pprcScan++)
        {
            RECT rcCheckOverlap;
            LPRECT lprcScan = *pprcScan;
            LPRECT FAR *pprcCheckOverlap;
            LPRECT FAR *FAR *pppBest;
            LPPOINT pdpBest;
            UINT FAR *pdBest;
            UINT dX, dY;
            UINT dTotal;

            //
            // Figure out how far the rectangle may be along both axes.
            // Note some of these numbers could be garbage at this point but
            // the code below will take care of it.
            //
            if (lprcScan->right <= lprcTest->left)
                dpMove.x = dX = lprcTest->left - lprcScan->right;
            else
                dpMove.x = -(int)(dX = (lprcScan->left - lprcTest->right));

            if (lprcScan->bottom <= lprcTest->top)
                dpMove.y = dY = lprcTest->top - lprcScan->bottom;
            else
                dpMove.y = -(int)(dY = (lprcScan->top - lprcTest->bottom));

            //
            // Figure out whether the rectangles are vertical, horizontal or
            // diagonal to each other and pick the measurements we will test.
            //
            if ((lprcScan->top < lprcTest->bottom) &&
                (lprcScan->bottom > lprcTest->top))
            {
                // The rectangles are somewhat horizontally aligned.
                dpMove.y = dY = 0;
                pppBest = &pprcAxis;
                pdpBest = &dpAxis;
                pdBest = &dAxis;
            }
            else if ((lprcScan->left < lprcTest->right) &&
                (lprcScan->right > lprcTest->left))
            {
                // The rectangles are somewhat vertically aligned.
                dpMove.x = dX = 0;
                pppBest = &pprcAxis;
                pdpBest = &dpAxis;
                pdBest = &dAxis;
            }
            else
            {
                // The rectangles are somewhat diagonally aligned.
                pppBest = &pprcDiag;
                pdpBest = &dpDiag;
                pdBest = &dDiag;
            }

            //
            // Make sure there aren't other rectangles in the way.  We only
            // need to check the upper array since that is the pool of
            // semi-placed rectangles.  Any rectangles in the lower array that
            // are "in the way" will be found in a different iteration of the
            // enclosing loop.
            //

            CopyRect(&rcCheckOverlap, lprcScan);
            OffsetRect(&rcCheckOverlap, dpMove.x, dpMove.y);

            for (pprcCheckOverlap = pprcScan + 1; pprcCheckOverlap < pprcL;
                pprcCheckOverlap++)
            {
                RECT rc;
                if (IntersectRect(&rc, *pprcCheckOverlap, &rcCheckOverlap))
                    break;
            }
            if (pprcCheckOverlap < pprcL)
            {
                // There was another rectangle in the way; don't use this one.
                continue;
            }

            //
            // If it is closer than the one we already had, use it instead.
            //
            dTotal = dX + dY;
            if (dTotal < *pdBest)
            {
                *pdBest = dTotal;
                *pdpBest = dpMove;
                *pppBest = pprcScan;
            }
        }
    }

    //
    // If we found anything along an axis use that otherwise use a diagonal.
    //
    if (dAxis != (UINT)-1)
    {
        pprcSplit = pprcAxis;
        dpMove = dpAxis;
    }
    else if (dDiag != (UINT)-1)
    {
        // BUGBUG: consider moving the rectangle to a side in this case.
        // (that, of course would add a lot of code to avoid collisions)
        pprcSplit = pprcDiag;
        dpMove = dpDiag;
    }
    else
        dpMove.x = dpMove.y = 0;

    //
    // Move the monitor into place and return it as the one we chose.
    //
    if (dpMove.x || dpMove.y)
        OffsetRect(*pprcSplit, dpMove.x, dpMove.y);

    return pprcSplit;
}

// ----------------------------------------------------------------------------
//
//  RemoveGaps()
//    This is called from CleanupDesktopRectangles to make sure the monitor
//  array is contiguous.  It assumes that the array is already non-overlapping.
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RemoveGaps(LPRECT arc, UINT count)
{
    LPRECT aprc[MONITORS_MAX];
    LPRECT lprc, lprcL, lprcSwap, FAR *pprc, FAR *pprcNearest;
    UINT uNearest;

    //
    // We will need to find the rectangle closest to the center of the group.
    // We don't really need to center the array here but it doesn't hurt and
    // saves us some code below.
    //
    CenterRectangles(arc, count);

    //
    // Build an array of LPRECTs we can shuffle around with relative ease while
    // not disturbing the order of the passed array.  Also take note of which
    // one is closest to the center so we start with it and pull the rest of
    // the rectangles inward.  This can make a big difference in placement when
    // there are more than 2 rectangles.
    //
    uNearest = (UINT)-1;
    pprc = aprc;
    lprcL = (lprc = arc) + count;

    while (lprc < lprcL)
    {
        int x, y;
        UINT u;

        //
        // Fill in the array.
        //
        *pprc = lprc;

        //
        // Check if this one is closer to the center of the group.
        //
        x = RectCenterX(lprc);
        y = RectCenterY(lprc);
        if (x < 0) x *= -1;
        if (y < 0) y *= -1;

        u = (UINT)x + (UINT)y;
        if (u < uNearest)
        {
            uNearest    = u;
            pprcNearest = pprc;
        }

        pprc++;
        lprc++;
    }

    //
    // Now make sure we move everything toward the centermost rectangle.
    //
    if (pprcNearest != aprc)
    {
        lprcSwap     = *pprcNearest;
        *pprcNearest = *aprc;
        *aprc        = lprcSwap;
    }

    //
    // Finally, loop through the array closing any gaps.
    //
    pprc = aprc + 1;
    for (lprc = arc + 1; lprc < lprcL; pprc++, lprc++)
    {
        //
        // Find the next suitable rectangle to combine into the group and move
        // it into position.
        //
        pprcNearest = AddNextContiguousRectangle(aprc, pprc, count);

        //
        // If the rectangle that was added is not the next in our array, swap.
        //
        if (pprcNearest != pprc)
        {
            lprcSwap     = *pprcNearest;
            *pprcNearest = *pprc;
            *pprc        = lprcSwap;
        }
    }
}

// ----------------------------------------------------------------------------
//
//  CleanUpDesktopRectangles()
//    This is called by CleanUpMonitorRectangles (etc) to force a set of
//  rectangles into a contiguous, non-overlapping arrangement.
//
// ----------------------------------------------------------------------------

BOOL
AlignRects(LPRECT arc, DWORD cCount, DWORD iPrimary, DWORD dwFlags)
{
    LPRECT lprc, lprcL;

    //
    // Limit for loops.
    //

    lprcL = arc + cCount;

    //
    // We don't need to get all worked up if there is only one rectangle.
    //

    if (cCount > MONITORS_MAX)
    {
        return FALSE;
    }


    if (cCount > 1)
    {
        if (!(dwFlags & CUDR_NOSNAPTOGRID))
        {
            //
            // Align monitors on 8 pixel boundaries so GDI can use the same
            // brush realization on compatible devices (BIG performance win).
            // Note that we assume the size of a monitor will be in multiples
            // of 8 pixels on X and Y.  We cannot do this for the work areas so
            // we convert them to be relative to the origins of their monitors
            // for the time being.
            //
            // The way we do this alignment is to just do the overlap/gap
            // resoluton in 8 pixel space (ie divide everything by 8 beforehand
            // and multiply it by 8 afterward).
            //
            // Note: WE CAN'T USE MULTDIV HERE because it introduces one-off
            // errors when monitors span the origin.  These become eight-off
            // errors when we scale things back up and we end up trying to
            // create DCs with sizes like 632x472 etc (not too good).  It also
            // handles rounding the wierdly in both positive and negative space
            // and we just want to snap things to a grid so we compensate for
            // truncation differently here.
            //
            for (lprc = arc; lprc < lprcL; lprc++)
            {
                RECT rc;
                int d;


                CopyRect(&rc, lprc);

                d = rc.right - rc.left;

                if (rc.left < 0)
                    rc.left -= 4;
                else
                    rc.left += 3;

                rc.left /= 8;
                rc.right = rc.left + (d / 8);

                d = rc.bottom - rc.top;

                if (rc.top < 0)
                    rc.top -= 4;
                else
                    rc.top += 3;

                rc.top /= 8;
                rc.bottom = rc.top + (d / 8);

                CopyRect(lprc, &rc);
            }
        }

        //
        // RemoveGaps is designed assuming that none of the rectangles that it
        // is passed will overlap.  Thus we cannot safely call it if we have
        // skipped the call to RemoveOverlaps or it might loop forever.
        //
        if (!(dwFlags & CUDR_NORESOLVEPOSITIONS))
        {
            RemoveOverlaps(arc, cCount);

            if (!(dwFlags & CUDR_NOCLOSEGAPS))
            {
                RemoveGaps(arc, cCount);
            }
        }

        if (!(dwFlags & CUDR_NOSNAPTOGRID))
        {
            //
            // Now return the monitor rectangles to pixel units this is a
            // simple multiply and MultDiv doesn't offer us any code size
            // advantage so (I guess that assumes a bit about the compiler,
            // but...) just do it right here.
            //
            for (lprc = arc; lprc < lprcL; lprc++)
            {
                lprc->left   *= 8;
                lprc->top    *= 8;
                lprc->right  *= 8;
                lprc->bottom *= 8;
            }
        }
    }

    if (!(dwFlags & CUDR_NOPRIMARY))
    {
        //
        // Reset all the coordinates based on the primaries position,
        // so that it is always located at 0,0
        //

        LONG dx = -((arc + iPrimary)->left);
        LONG dy = -((arc + iPrimary)->top);

        for (lprc = arc; lprc < lprcL; lprc++)
        {
            OffsetRect(lprc, dx, dy);
        }
    }

    return TRUE;
}
