// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//

#include "precomp.hpp"


//+------------------------------------------------------------------------
//
//  Function:  
//      AreTransformedRectanglesClose
//
//  Synopsis:  
//      Determines whether or not two rectangles are within a specified
//      distance of each other, after being transformed to another
//      coordinate space.
//
//-------------------------------------------------------------------------  
BOOL
AreTransformedRectanglesClose(
    __in_ecount(1) const CMilRectF *pFirstRect,
        // Rectangle to compare.  The order of pFirstRect & pSecondRect doesn't matter.
    __in_ecount(1) const CMilRectF *pSecondRect,
        // The other rectangle to compare.  The order of pFirstRect & pSecondRect doesn't matter. 
    __in_ecount_opt(1) const CMILMatrix *pTransform,
        // Transform to apply to both rectangles before determining how close they are
        // to each other.
    __in float closeTolerance
        // The maximum distance the mapped rectangle edges can be before they are no
        // longer 'close'.
    )
{
    enum { TOP_LEFT = 0,
           TOP_RIGHT = 1,
           BOTTOM_RIGHT = 2,
           BOTTOM_LEFT = 3,
           NUM_VECTORS };

    MilPoint2F differenceVectors[NUM_VECTORS];

    BOOL fAreClose = TRUE;
    float toleranceSquared  = closeTolerance * closeTolerance;    

    //
    // Compute the difference vectors between the 4 points of both rectangles
    //
    // To avoid transforming both rectangles, we instead transform their difference 
    // vectors, which reduces the number of points that need to be transformed 
    // in half (from 8 to 4).
    //

    // Compute the difference vector between the top-left points
    differenceVectors[TOP_LEFT].X = pFirstRect->left - pSecondRect->left;
    differenceVectors[TOP_LEFT].Y = pFirstRect->top - pSecondRect->top;        

    // Compute the difference vector between the bottom-right points
    differenceVectors[BOTTOM_RIGHT].X = pFirstRect->right - pSecondRect->right;
    differenceVectors[BOTTOM_RIGHT].Y = pFirstRect->bottom - pSecondRect->bottom;

    // Use the already-computed differences of the top-left & bottom-right to obtain 
    // the difference between the top-right points
    differenceVectors[TOP_RIGHT].X = differenceVectors[BOTTOM_RIGHT].X;
    differenceVectors[TOP_RIGHT].Y = differenceVectors[TOP_LEFT].Y;

    // Use the already-computed differences of the top-left & bottom-right to obtain 
    // the difference between the bottom-left points
    differenceVectors[BOTTOM_LEFT].X = differenceVectors[TOP_LEFT].X;
    differenceVectors[BOTTOM_LEFT].Y = differenceVectors[BOTTOM_RIGHT].Y;

    //
    // Transform the difference vectors 
    //
    if (pTransform)
    {    
        pTransform->TransformAsVectors(differenceVectors, differenceVectors, NUM_VECTORS);
    }

    //
    // Determine if the difference between the transformed rectangles is larger
    // than the tolerance.
    //

    // Evaluate the difference vectors for all 4 points
    for (UINT i = 0; i < NUM_VECTORS; i++)
    {
        // To avoid using sqrt(), we square both sides of the magnitude equation, 
        // resulting in us comparing the terms underneath the sqrt to the square 
        // of the tolerance.
        float magnitudeSquared = differenceVectors[i].X * differenceVectors[i].X + differenceVectors[i].Y * differenceVectors[i].Y;

        if (magnitudeSquared > toleranceSquared)
        {
            fAreClose = FALSE;
            break;
        }
    }    

    return fAreClose;
}


//-----------------------------------------------------------------------------
//  Function:  
//      RectF_RBFromParallelogramPointsF
//
//  Synopsis:  
//      Helper function to take 4 points corresponding to a parallelogram
//      and detect if it's a rectangle. If so, it will fill in the rect.
//
//-----------------------------------------------------------------------------

BOOL
RectF_RBFromParallelogramPointsF(
    __in_ecount(4) const MilPoint2F *pPoints,
    __out_ecount_opt(1) MilRectF *pRectF_RB
    )
{
    //
    // The points can start at either 0, 1, 2, or 3 and then be ordered in
    // either clock-wise or counter-clockwise order. It is assumed that
    // the points form a parallelogram.
    //     Examples:
    //       0--------1         1--------2
    //      /        /          |        |
    //     3--------2           0--------3
    //
    // If two sides of the parallelogram are axis-aligned, then the other 
    // sides must also be axis-aligned, making the figure a rectangle.
    //

    if ((pPoints[0].X == pPoints[3].X) && (pPoints[0].Y == pPoints[1].Y))
    {
        //
        // With the assumption that the points are already a parallelogram,
        // the points, have been validated to be a rectangle.
        //

        if (pRectF_RB)
        {
            if (pPoints[0].Y < pPoints[3].Y)
            {
                pRectF_RB->top = pPoints[0].Y;
                pRectF_RB->bottom = pPoints[3].Y;
            }
            else
            {
                pRectF_RB->top = pPoints[3].Y;
                pRectF_RB->bottom = pPoints[0].Y;
            }

            if (pPoints[0].X < pPoints[1].X)
            {
                pRectF_RB->left = pPoints[0].X;
                pRectF_RB->right = pPoints[1].X;
            }
            else
            {
                pRectF_RB->left = pPoints[1].X;
                pRectF_RB->right = pPoints[0].X;
            }
        }

        return TRUE;
    }
    else if ((pPoints[0].Y == pPoints[3].Y) && (pPoints[0].X == pPoints[1].X))
    {
        //
        // With the assumption that the points are already a parallelogram,
        // the points, have been validated to be a rectangle.
        //

        if (pRectF_RB)
        {
            if (pPoints[0].Y < pPoints[1].Y)
            {
                pRectF_RB->top = pPoints[0].Y;
                pRectF_RB->bottom = pPoints[1].Y;
            }
            else
            {
                pRectF_RB->top = pPoints[1].Y;
                pRectF_RB->bottom = pPoints[0].Y;
            }

            if (pPoints[0].X < pPoints[3].X)
            {
                pRectF_RB->left = pPoints[0].X;
                pRectF_RB->right = pPoints[3].X;
            }
            else
            {
                pRectF_RB->left = pPoints[3].X;
                pRectF_RB->right = pPoints[0].X;
            }
        }

        return TRUE;
    }
    else
    {
        // The points are not a rectangle
        return FALSE;
    }
}



