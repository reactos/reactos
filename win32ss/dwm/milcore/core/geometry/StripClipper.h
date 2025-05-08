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

//+-----------------------------------------------------------------------------
//
//  Class:
//      CStripClipper
//
//  Synopsis:
//      Clips a geometry to a given strip of space, defined as the interior of
//      two parallel lines.
//
//  NOTE:
//      Like the scanner, we only guarantee the _fill_ of the geometry to be
//      correct -- the stroke may be completely wrong. In particular, the
//      clipper may introduce edges along the boundary of the clip rect that
//      were not there before. We will also introduce a closing edge, even if
//      the input figure is not closed (the output figure will be marked
//      "closed", though, iff the input figure was marked closed).
//
//  NOTE:
//      Unlike the scanner, no curve reconstruction is ever performed. Shapes
//      that contain beziers will be flattened. Hence, it is advised that
//      clipping occur in device space.
//
//  NOTE:
//      This class isn't as numerically stable as CAxisAlignedStripClipper (the
//      outputted geometry need not lie strictly inside the bounds provided,
//      especially if the passed in geometry is massive). It may also be a
//      little slower.  If you are performing axis-aligned clipping, you're
//      probably better off using CAxisAlignedClipper.
//
//  Algorithm description:
//      ----------------------
//
//      The lines passed in during construction divide space up into 3 regions,
//      which we designate negative, inside, and positive.  During population,
//      as long as the geometry remains in the "inside region", we pass along
//      the verticies as is.  Whenever a figure leaves the inside region,
//      though, we calculate the intersection point and pass that along instead.
//      Later, when the figure re-enters the inside region, we pass along the
//      point at re-entry.
//
//      We can thus think about the algorithm as replacing portions of the
//      figure that occur outside clip region with equivalent line segments
//      along the clip region's boundaries. Our ability to do this crucially
//      depends on the fact that it's impossible to encircle the clip region
//      without passing through it. Consider trying to clip a shape to a rect in
//      one pass:
//
//                                * ****
//                               *      *
//                              *        *****
//                              *             *
//                         |--- A ---------|   *
//                         |    *..........|   *
//                         |    *../\......|   *
//                         |     *.||......|   *  ||
//          Clip rect ->   |     *.........|   *  \/
//                         |    *..........|   *
//                         |    *..........|   *
//                         |    *..........|   *  <- Figure to be clipped
//                         |--- B ---------|   *
//                              *              *
//                               *            **
//                                ************
//
//      . = Filled region
//
//      Upon leaving the clip region at A (at the top of the clip rect), we
//      would somehow need to sense that the figure proceeded around the clip
//      rect in a clock-wise direction and replace it with segments that traced
//      along the top edge, then the right edge, and then the bottom edge to B.
//      The situation is even worse when the figure enters and leaves the clip
//      rect many times.
//
//------------------------------------------------------------------------------

enum PointRegion
{
    PointRegionNegative = 0,
    PointRegionInside = 1,
    PointRegionPositive = 2,
    PointRegionInvalid = 3
};

class CStripClipper : public IPopulationSink, CFlatteningSink
{
public:
    // The passed in geometry will be clipped to the region falling between the
    // lines:
    //      a*x + b*y = c
    //      a*x + b*y = d
    // 
    CStripClipper(
        double a,
        double b,
        double c,
        double d,
        __in_ecount(1) IPopulationSink *pSink,
            // The recepient of the result of the operation
        double rTolerance=0)
            // Curve retrieval error tolerance
        : m_a(a), m_b(b), m_c(c), m_d(d), m_fFirstPointAdded(false),
          m_lastPoint(0,0), m_startPoint(0,0), m_pSink(pSink),
          m_rTolerance(rTolerance), m_lastPointRegion(PointRegionInvalid)
    {
        Assert(m_pSink != NULL);

        if (m_c > m_d)
        {
            // Swap
            double tmp = m_c; m_c = m_d; m_d = tmp;
        }
    }
    
protected:
    CStripClipper(
        double c,
        double d,
        __inout_ecount(1) IPopulationSink *pSink,
            // The recepient of the result of the operation
        double rTolerance=0)
            // Curve retrieval error tolerance
        : m_c(c), m_d(d), m_fFirstPointAdded(false),
          m_lastPoint(0,0), m_startPoint(0,0), m_pSink(pSink),
          m_rTolerance(rTolerance), m_lastPointRegion(PointRegionInvalid)
    {
        Assert(m_pSink != NULL);

        if (m_c > m_d)
        {
            // Swap
            double tmp = m_c; m_c = m_d; m_d = tmp;
        }
    }

public:
    virtual ~CStripClipper()
    {
    }

    //
    // IPopulationSink methods
    //
    
    virtual HRESULT StartFigure(
        __in_ecount(1) const GpPointR &pt);
            // Figure's first point

    virtual HRESULT AddLine(
        __in_ecount(1) const GpPointR &ptNew);
            // The line segment's endpoint

    virtual HRESULT AddCurve(
        __in_ecount(3) const GpPointR *rgPoints);
            // The last 3 Bezier points of the curve (we already have the first one)

    virtual void SetCurrentVertexSmooth(bool val)
    {
        if (GetPointRegion(m_lastPoint) == PointRegionInside)
        {
            m_pSink->SetCurrentVertexSmooth(val);
        }
    }

    virtual void SetStrokeState(bool val)
    {
        m_pSink->SetStrokeState(val);
    }

    virtual HRESULT EndFigure(
        bool fClosed);
            // =true if the figure is closed

    virtual void SetFillMode(
        MilFillMode::Enum eFillMode) // The mode that defines the fill set
    {
        m_pSink->SetFillMode(eFillMode);
    }

    //
    // CFlatteningSink method
    //

    virtual HRESULT AcceptPoint(
        __in_ecount(1) const GpPointR &pt,
            // The point
        GpReal t,
            // Parameter we're at
        __out_ecount(1) bool &fAbort);
            // Set to true to signal aborting

protected:
     virtual GpPointR GetIntersectionWithBound(
        __in_ecount(1) const GpPointR &pt1,
            // first point on line segment
        __in_ecount(1) const GpPointR &pt2,
            // last point on line segment
        PointRegion side);
            // side of clip rect to intersect with
    
private:

    PointRegion GetPointRegion(
        __in_ecount(1) const GpPointR &pt) const;
            // Point to be classified

    HRESULT AddPoint(
        __in_ecount(1) const GpPointR &pt);
            // The new point to add

    HRESULT AddIntersectionPointsOnSegment(
        __in_ecount(1) const GpPointR &pt1,
            // Start of segment
        PointRegion ePtRegion1,
            // Region pt1 belongs to
        __in_ecount(1) const GpPointR &pt2,
            // End of segment
        PointRegion ePtRegion2,
            // Region pt2 belongs to
        bool fIncludePt2);
            // Should we additionally add Pt2 if it falls inside the region?

private:

    bool m_fFirstPointAdded;
        // Have we actually added a point to our sink yet?
    GpPointR m_startPoint;
        // Start of the figure passed in (not necessarily inside the clip
        // region)
    GpPointR m_lastPoint;
        // The last point we've encountered so far (not necessarily inside the
        // clip region)
    PointRegion m_lastPointRegion;
        // The region m_lastPoint belongs to.
    IPopulationSink *m_pSink;
        // Sink to output figures to.

    double m_rTolerance;
        // Tolerance of bezier flattener.

    // Line bounds parameters
protected:
    double m_a;
    double m_b;
    double m_c;
    double m_d;
};


