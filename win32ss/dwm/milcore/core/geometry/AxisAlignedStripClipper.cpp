// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//
//  $Description:
//      Definition of CAxisAlignedStripClipper
//
//  $ENDTAG
//
//  Classes:
//      CAxisAlignedStripClipper.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAxisAlignedStripClipper::GetIntersectionWithBound
//
//  Synopsis:
//      Determine the intersection of the boundary line determined by side and
//      the line determined by the points pt1 and pt2.
//
//  Notes:
//      It is an error to pass in points pt1 and pt2 that lie on the same side
//      of the line.
//
//------------------------------------------------------------------------------
GpPointR CAxisAlignedStripClipper::GetIntersectionWithBound(
    __in_ecount(1) const GpPointR &pt1,
        // first point on line segment
    __in_ecount(1) const GpPointR &pt2,
        // last point on line segment
    PointRegion side)
        // side of clip region to intersect with
{
    double x, y, c;

    Assert(side == PointRegionNegative || side == PointRegionPositive);
    c = (side == PointRegionNegative) ? m_c : m_d;

    //
    // If the line defined by pt1 and pt2 is close to parallel with the
    // boundary line, the denominator in the following will be close to 0.
    // Since pt1 and pt2 are on either side of bounds, though, the lines cannot
    // be perfectly parallel. 
    //

    if (m_fVerticalBounds)
    {
        x = c;
        y = ( pt1.Y * (c - pt2.X) - pt2.Y * (c - pt1.X) ) / (pt1.X - pt2.X);
    }
    else
    {
        x = ( pt1.X * (c - pt2.Y) - pt2.X * (c - pt1.Y) ) / (pt1.Y - pt2.Y);
        y = c;
    }

    return (GpPointR(x,y));
}



