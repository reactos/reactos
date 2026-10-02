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

//+-----------------------------------------------------------------------------
//
//  Class:
//      CAxisAlignedStripClipper
//
//  Synopsis:
//      Clips a geometry to a given strip of space, defined as the interior of
//      two axis-aligned parallel lines. More numerically stable than
//      CStripClipper.
//
//  In particular, CAxisAlignedStripClipper satisfies the following invariants:
//
//  Assuming vertical (horizontal) bounds are specified:
//      1) If an input point is specified whose x-value (y-value) lies in the
//         closed interval [c,d], then that point will exist in the output.
//      2) Regardless of inputs, the x-values (y-values) of the output
//         verticies will lie in the closed interval [c, d]. No guarantess are
//         made on the y-values (x-values), however. Indeed, they may well be
//         NaNs.
//
//      See StripClipper.h for implementation details.
//
//------------------------------------------------------------------------------

class CAxisAlignedStripClipper : public CStripClipper
{
public:
    // The passed in geometry will be clipped to the region falling between the
    // lines:
    // 
    // fVerticalBounds:    true       false
    //                     ----------------
    //                     x = c      y = c 
    //                     x = d      y = d
    // 
    CAxisAlignedStripClipper(
        bool fVerticalBounds,
        double c,
        double d,
        __in_ecount(1) IPopulationSink *pSink,
            // The recepient of the result of the operation
        double rTolerance=0)
            // Curve retrieval error tolerance
        : m_fVerticalBounds(fVerticalBounds), 
          CStripClipper(c, d, pSink, rTolerance)
    {
        if (fVerticalBounds)
        {
            m_a = 1.0;
            m_b = 0.0;
        }
        else
        {
            m_a = 0.0;
            m_b = 1.0;
        }
    }

    virtual ~CAxisAlignedStripClipper()
    {
    }

protected:

    virtual GpPointR GetIntersectionWithBound(
        __in_ecount(1) const GpPointR &pt1,
            // first point on line segment
        __in_ecount(1) const GpPointR &pt2,
            // last point on line segment
        PointRegion side);
            // side of clip rect to intersect with

private:

    // Line bounds parameters
    bool m_fVerticalBounds;
        // Are the bounding lines vertical or horizontal?
};


