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
//      Definition of CStripClipper
//
//  $ENDTAG
//
//  Classes:
//      CStripClipper.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStripClipper::StartFigure
//
//  Synopsis:
//      Initiate a new figure, specifying the start point.
//
//  Notes:
//      We do not currently handle curve retrieval. It is an error to pass in a
//      curve to this method.
//
//------------------------------------------------------------------------------
HRESULT CStripClipper::StartFigure(
    __in_ecount(1) const GpPointR &pt)
        // Figure's first point
{
    HRESULT hr = S_OK;
    PointRegion ePtRegion = GetPointRegion(pt);

    m_fFirstPointAdded = false;

    if (ePtRegion == PointRegionInside)
    {
        IFC(AddPoint(pt));
    }

    m_startPoint = pt;
    m_lastPoint = pt;
    m_lastPointRegion = ePtRegion;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStripClipper::AddLine
//
//  Synopsis:
//      Add a new line segment to the currently active figure.
//
//------------------------------------------------------------------------------
HRESULT CStripClipper::AddLine(
    __in_ecount(1) const GpPointR &ptNew)
        // Endpoint of the new line segment.
{
    HRESULT hr = S_OK;

    PointRegion ptNewRegion = GetPointRegion(ptNew);

    IFC(AddIntersectionPointsOnSegment(
            m_lastPoint,
            m_lastPointRegion,
            ptNew,
            ptNewRegion,
            true /* include ptNew */));

    m_lastPoint = ptNew;
    m_lastPointRegion = ptNewRegion;
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStripClipper::AddCurve
//
//  Synopsis:
//      Add a new bezier segment to the currently active figure.
//
//------------------------------------------------------------------------------
HRESULT CStripClipper::AddCurve(
    __in_ecount(3) const GpPointR *rgPoints)
        // The last 3 Bezier points of the curve (we already have the first one)
{
    HRESULT hr = S_OK;

    // Future Consideration:  It's possible to operate on the Bezier
    // directly without flattening it. Among other things, this could make the
    // clipper resolution independent.

    if (m_lastPointRegion == GetPointRegion(rgPoints[2]) &&
        m_lastPointRegion == GetPointRegion(rgPoints[1]) &&
        m_lastPointRegion == GetPointRegion(rgPoints[0]))
    {
        if (m_lastPointRegion == PointRegionInside)
        {
            IFC(m_pSink->AddCurve(rgPoints));
        }
        // Else, the curve is entirely outside the clip region and we can
        // ignore it.

        // Note that m_lastPointRegion doesn't change.
        m_lastPoint = rgPoints[2];
    }
    else
    {
        CBezierFlattener flattener(this, m_rTolerance);

        flattener.SetPoint(0, m_lastPoint);
        flattener.SetPoint(1, rgPoints[0]);
        flattener.SetPoint(2, rgPoints[1]);
        flattener.SetPoint(3, rgPoints[2]);

        IFC(flattener.Flatten(false /* => no tangents */));
    }

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStripClipper::EndFigure
//
//  Synopsis:
//      Signal the end of the current figure.
//
//------------------------------------------------------------------------------
HRESULT CStripClipper::EndFigure(
    bool fClosed)
        // =true if the figure is closed
{
    HRESULT hr = S_OK;

    // if we've gone through the entire figure and haven't entered the strip,
    // we can just ignore it.
    if (m_fFirstPointAdded)
    {
        // Draw a line back to the beginning, m_startPoint's already been taken
        // care of, so don't include it.
        IFC(AddIntersectionPointsOnSegment(
                m_lastPoint,
                m_lastPointRegion,
                m_startPoint,
                GetPointRegion(m_startPoint),
                false /* don't include m_startPoint */
                ));

        Assert(m_pSink != NULL);
        IFC(m_pSink->EndFigure(fClosed));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStripClipper::AcceptPoint
//
//  Synopsis:
//      Add a new point from the flattener
//
//------------------------------------------------------------------------------
HRESULT CStripClipper::AcceptPoint(
    __in_ecount(1) const GpPointR &pt,
        // The point
    GpReal t,
        // Parameter we're at
    __out_ecount(1) bool &fAbort)
        // Set to true to signal aborting
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(t);

    fAbort = false;

    IFC(AddLine(pt));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStripClipper::GetPointRegion
//
//  Synopsis:
//      Determine in which region (defined by the two boundary lines) pt is.
//
//------------------------------------------------------------------------------
PointRegion CStripClipper::GetPointRegion(
    __in_ecount(1) const GpPointR &pt) const
        // Point to be classified
{
    PointRegion region;
    
    // Ignore NaNs.
    Assert(!(m_c > m_d));

    double r = m_a*pt.X + m_b*pt.Y;

    if (r < m_c)
    {
        region = PointRegionNegative;
    }
    else if (r <= m_d)
    {
        region = PointRegionInside;
    }
    else
    {
        region = PointRegionPositive;
    }

    return region;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStripClipper::GetIntersectionWithBound
//
//  Synopsis:
//      Determine the intersection of the boundary line determined by side and
//      the line determined by the points pt1 and pt2.
//
//  Notes:
//      It is an error to pass in points pt1 and pt2 that lie on the same side
//      of the line. If such points are passed in, or if we encounter numerical
//      errors, we will output pt1 as the intersection point.
//
//------------------------------------------------------------------------------
GpPointR CStripClipper::GetIntersectionWithBound(
    __in_ecount(1) const GpPointR &pt1,
        // first point on line segment
    __in_ecount(1) const GpPointR &pt2,
        // last point on line segment
    PointRegion side)
        // side of clip region to intersect with
{
    double t, c;

    Assert(side == PointRegionNegative || side == PointRegionPositive);
    c = (side == PointRegionNegative) ? m_c : m_d;

    // Define intersection point (x,y) = (t*x1 + (1-t)*x2, t*y1 + (1-t)*y2)
    // 
    // We wish to solve:
    //
    // m_a*x + m_b*y == c
    //
    // If the line defined by pt1 and pt2 is close to parallel with the
    // boundary line, the denominator in the following will be close to 0.
    // Since pt1 and pt2 are on either side of bounds, though, the lines cannot
    // be perfectly parallel. 
    //
    // Future Consideration:  It may be worth refactoring this code so that
    // we calculate this quantity simultaneously for m_c and m_d.

    t = ( c - m_a*pt2.X - m_b*pt2.Y ) / ( m_a*(pt1.X - pt2.X) + m_b*(pt1.Y - pt2.Y) );

    // Due to numerical issues, t may not be strictly between 0 and 1 -- it may
    // even be Inf or NaN if the line segment is short and close to parallel
    // with the boundary. In these cases, though, it is reasonable to simply
    // clamp between 0 and 1 (mapping NaN arbitrarily to 0).

    t = ClampDouble(t, 0, 1);

    return (GpPointR(t*pt1.X + (1-t)*pt2.X,
                     t*pt1.Y + (1-t)*pt2.Y));
}


HRESULT CStripClipper::AddPoint(
    __in_ecount(1) const GpPointR &pt)
        // The new point to add
{
    HRESULT hr = S_OK;

    if (m_fFirstPointAdded)
    {
        IFC(m_pSink->AddLine(pt));
    }
    else
    {
        m_fFirstPointAdded = true;
        IFC(m_pSink->StartFigure(pt));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStripClipper::AddIntersectionPointsOnSegment
//
//  Synopsis:
//      Add all the points on the segment that intersect the clip lines.
//      Optionally, also add Pt2 if it falls inside the clip bounds. pt1 will
//      never be added (since it has already been taken care of by the previous
//      segment).
//
//------------------------------------------------------------------------------
HRESULT CStripClipper::AddIntersectionPointsOnSegment(
    __in_ecount(1) const GpPointR &pt1,
        // Start of segment
    PointRegion ePtRegion1,
        // Region pt1 belongs to
    __in_ecount(1) const GpPointR &pt2,
        // End of segment
    PointRegion ePtRegion2,
        // Region pt2 belongs to
    bool fIncludePt2)
        // Should we additionally add Pt2 if it falls inside the region?
{
    HRESULT hr = S_OK;

    if (ePtRegion1 == PointRegionInside)
    {
        if (ePtRegion2 == PointRegionInside)
        {
            if (fIncludePt2)
            {
                IFC(AddPoint(pt2));
            }
        }
        else
        {
            IFC(AddPoint(GetIntersectionWithBound(pt1, pt2, ePtRegion2)));
        }
    }
    else
    {
        if (ePtRegion2 == PointRegionInside)
        {
            IFC(AddPoint(GetIntersectionWithBound(pt1, pt2, ePtRegion1)));

            if (fIncludePt2)
            {
                IFC(AddPoint(pt2));
            }
        }
        else if (ePtRegion1 != ePtRegion2)
        {
            IFC(AddPoint(GetIntersectionWithBound(pt1, pt2, ePtRegion1)));
            IFC(AddPoint(GetIntersectionWithBound(pt1, pt2, ePtRegion2)));
        }
    }

Cleanup:
    RRETURN(hr);
}


