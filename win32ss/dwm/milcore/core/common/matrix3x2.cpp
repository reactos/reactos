// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Implementation of the light weight 2D matrix class.
//

#include "precomp.hpp"


//+----------------------------------------------------------------------------
//
//  Member:    MILMatrix3x2::TransformPoints
//
//  Synopsis:  Transform an array of points using the matrix v' = v M:
//
//                                            [ M00 M01 0 ]
//                [vx', vy', 1] = [vx, vy, 1] [ M10 M11 0 ]
//                                            [ dx  dy  1 ]
//

void MILMatrix3x2::TransformPoints(
    __in_ecount(count) const MilPoint2F *srcPoints,
    __out_ecount(count) MilPoint2F *destPoints,
    UINT count
    ) const
{
    do {
        REAL x = srcPoints->X;
        REAL y = srcPoints->Y;

        destPoints->X = (m_00 * x) + (m_10 * y) + m_20;
        destPoints->Y = (m_01 * x) + (m_11 * y) + m_21;

    } while (destPoints++, srcPoints++, --count != 0);
}


//+----------------------------------------------------------------------------
//
//  Member:    MILMatrix3x2::Transform2DBounds
//
//  Synopsis:  Transform 2D rectangle bounds using the matrix v' = v M:
//
//                                            [ M00 M01 0 ]
//                [vx', vy', 1] = [vx, vy, 1] [ M10 M11 0 ]
//                                            [ dx  dy  1 ]
//
//             for each corner and produce a bounding rectangle for those
//             results.
//
//             Since Transform2DBounds works by transforming each corner
//             individually it expects that incoming bounds fall within
//             reasonable floating point limits.
//

void MILMatrix3x2::Transform2DBounds(
    __in_ecount(1) const MilRectF &srcRect,
    __out_ecount(1) MilRectF &destRect
    ) const
{
    MilPoint2F pt[4];

    pt[0].X = srcRect.left;
    pt[0].Y = srcRect.top;
    pt[1].X = srcRect.right;
    pt[1].Y = srcRect.top;
    pt[2].X = srcRect.left;
    pt[2].Y = srcRect.bottom;
    pt[3].X = srcRect.right;
    pt[3].Y = srcRect.bottom;

    TransformPoints(&pt[0], &pt[0], ARRAYSIZE(pt));

    destRect.left = pt[0].X;
    destRect.top  = pt[0].Y;
    destRect.right  = pt[0].X;
    destRect.bottom = pt[0].Y;

    for (int i = 1; i < ARRAYSIZE(pt); i++)
    {
        if (pt[i].X < destRect.left)
        {
            destRect.left = pt[i].X;
        }
        else if (pt[i].X > destRect.right)
        {
            destRect.right = pt[i].X;
        }

        if (pt[i].Y < destRect.top)
        {
            destRect.top = pt[i].Y;
        }
        else if (pt[i].Y > destRect.bottom)
        {
            destRect.bottom = pt[i].Y;
        }
    }
}



