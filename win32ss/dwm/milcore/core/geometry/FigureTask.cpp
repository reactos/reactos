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
//      Helper classes for traversing a figure for various computations
//
//  $ENDTAG
//
//  Classes:
//      CFigureTask and its subclasses;  CMILBezierFlattener
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#pragma optimize("t", on)

///////////////////////////////////////////////////////////////////////////////
//
//      Implementation of CBoundsTask

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBoundsTask::DoLineNoHRESULT
//
//  Synopsis:
//      Update the bounds with a line segment. Guaranteed to succeed, so no
//      HRESULT.
//
//------------------------------------------------------------------------------
void
CBoundsTask::DoLineNoHRESULT(
    __in_ecount(1) const MilPoint2F &ptEnd
        // The line's end point
    )
{
    if (m_pMatrix)
    {
        TransformPoint(*m_pMatrix, ptEnd, m_ptCurrent);
    }
    else
    {
        m_ptCurrent = ptEnd;
    }
    m_oBounds.UpdateWithPoint(m_ptCurrent);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CBoundsTask::DoBezier
//
//  Synopsis:
//      Process a Bezier segment - CFigureTask override
//
//------------------------------------------------------------------------------
HRESULT 
CBoundsTask::DoBezier(
    __in_ecount(3) const MilPoint2F *pt
        // The missing 3 Bezier points
    )
{
    GpPointR ptBez[3];
    if (m_pMatrix)
    {
        for (int i = 0;  i < 3;  i++)
        {
            TransformPoint(*m_pMatrix, pt[i], ptBez[i]);
        }
    }
    else
    {
        for (int i = 0;  i < 3;  i++)
        {
            ptBez[i] = pt[i];
        }
    }

    m_oBounds.UpdateWithBezier(m_ptCurrent, ptBez[0], ptBez[1], ptBez[2]);
    m_ptCurrent = ptBez[2];
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
//      Implementation of CHitTest

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTest::CHitTest
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CHitTest::CHitTest(
    __in_ecount(1) const GpPointR &ptHit,
        // The hit test point (in transformed space)
    __in_ecount_opt(1) const CMILMatrix *pMatrix,
        // Transformation to apply to the figure (NULL OK)
    double rThreshold
        // Distance to be considered a hit
    )
    : m_iWinding(0), m_ptHit(ptHit)
{
    m_rSquaredThreshold = (rThreshold * rThreshold);
    
    // The winding number computation can produce an incorrect result if the hit point
    // is close to the boundary, so the following is necessary for the integrity of the
    // algorithm.
    if (m_rSquaredThreshold < SQ_LENGTH_FUZZ)
    {
        m_rSquaredThreshold = SQ_LENGTH_FUZZ;
    }
    
    if (pMatrix)
    {
        m_oMatrix = *pMatrix;
    }
    else
    {
        m_oMatrix.SetToIdentity();
    }
    
    // Set the transformation to shift the hit point to the origin
    m_oMatrix.Translate(REAL(-ptHit.X), REAL(-ptHit.Y));
}
  
//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTest::EndAt
//
//  Synopsis:
//      Process the figure's end point
//
//  Returns:
//      true if a hit was detected.
//
//------------------------------------------------------------------------------
bool
CHitTest::EndAt(
    __in_ecount(1) const MilPoint2F &ptFirst
        // The figure's end point
    )
{
    GpPointR pt = GpPointR(ptFirst, &m_oMatrix);
    
    Assert(!m_fAborted);  // Otherwise we should have aborted

    AcceptPointNoHRESULT(pt, 0.0, m_fAborted);

    return m_fAborted;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTest::AcceptPointNoHRESULT
//
//  Synopsis:
//      Process a segment. Guaranteed not to fail, so does not return HRESULT.
//
//------------------------------------------------------------------------------
void
CHitTest::AcceptPointNoHRESULT(
    __in_ecount(1) const GpPointR &ptEnd,
        // The segment's endpoint
    GpReal,
        // Ignored here
    __out_ecount(1) bool &fHit
        // Set to true when a hit is detected
    )
{
    Assert(!m_fAborted);   // Should have bailed out otherwise

    // This segment is close enough to the origin
    CheckIfNearTheOrigin(ptEnd);
    fHit = m_fAborted;

    if (!m_fAborted)
    {
        // Update the number of path intersections with the positive x axis
        UpdateWith(ptEnd);
    }

    m_ptCurrent = ptEnd;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTest::CheckIfNearTheOrigin
//
//  Synopsis:
//      Private method - check if this segment is near the origin
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE void 
CHitTest::CheckIfNearTheOrigin(
    __in_ecount(1) const GpPointR &ptEnd
        // The segment's endpoint
    )
{
    // Check if the endpoint is near the origin.  No need to check the start
    // point, it was checked as the endpoint of the previous segment
    m_fAborted = (ptEnd * ptEnd < m_rSquaredThreshold);

    if (!m_fAborted)
    {
        // Now check if there is point in the segment that is close enough to the origin
        // Let vec = ptEnd - m_ptCurrent be the segment vector.  The segment is 
        //  
        //      P(t) = m_ptCurrent + t*vec.
        //
        // If P(t) is the point on the line nearest to the origin then P(t) is 
        // perpendicular to segment, i.e. P(t) * vec = 0. The equation for t is then
        //
        //      (m_ptCurrent + t*vec) * vec = 0.
        //
        // The solution is
        //
        //      t = -(m_ptCurrent * vec) / (vec * vec)
        //
        // and it is inside the segment if 0 < t < 1. 
        //
        // The point at t is 
        //
        //      P = m_ptCurrent + ( (m_ptCurrent * vec) / (vec * vec) )*vec.
        //
        // and its squared distance from the origin is P * P.  If (0<t<=1) we
        // want to check if P * P < m_rSquaredThreshold.  But to avoid 
        // divisions, we'll set r = vec * vec, and multiply 0<t<=1 by r
        // and P * P < m_rSquaredThreshold by r*r.                 

        GpPointR vec(m_ptCurrent, ptEnd);
        double r = vec * vec;
        double t = -(m_ptCurrent * vec);
        if (0 <= t && t <= r)
        {
            // The nearest point is inside the segment, examine its distance
            GpPointR Pr = m_ptCurrent * r + vec * t;  //=P*r
            m_fAborted = Pr * Pr < m_rSquaredThreshold * r * r;
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTest::UpdateWith
//
//  Synopsis:
//      Private method - update the winding number with this point
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE void 
CHitTest::UpdateWith(
    __in_ecount(1) const GpPointR &ptEnd
        // The segment's endpoint
    )
{
    // The talying of crossing of the positive x axis may fail if the origin is
    // very closed to the (transformed) path, but then we'll be saved by the
    // nearness test, provided the tolerance is not too small.  So:
    Assert(!(m_rSquaredThreshold <= FUZZ)); // Ignore NaNs

    // If this segment crosses the x axis we have to determine whether it
    // does it at the positive half.  The x of the intersection is a weighted
    // average of the x coordinates of the segment's endpoints. By triangle
    // similarity, the ratio of the distances between the crossing x and
    // the x's of the endpoints is equal to |ptEnd.Y| / |m_ptCurrent.Y|.
    // 
    //
    //           * ptEnd
    //           |\
    //      *----*-\--*-------------
    //              \ |
    //               \|
    //                * m_ptCurrent
    //          
    // This translates to x = s * m_ptCurrent.X + t * ptEnd.X, where
    // s = |ptEnd.Y|/r, t = |m_ptCurrent.Y|/r, r = |m_ptCurrent.Y|+|ptEnd.Y|.
    // Since we are only interested in the sign of x, we can multiply that by 
    // r (which is known to be positive) and examine the sign of 
    // |m_ptCurrent.Y| * ptEnd.X + |ptEnd.Y| * m_ptCurrent.X.
    //
    // Instead of taking abs of both Y's we check their signs, which we need anyway,
    // and adjust them to be + when we test.

    if (m_ptCurrent.Y > 0)
    {
        if (ptEnd.Y <= 0)
        {
            // We have crossed the x axis going down
            if (m_ptCurrent.X * ptEnd.Y - ptEnd.X * m_ptCurrent.Y >= 0)
            {
                // The crossing was on the positive side
                m_iWinding--;
            }
        }
    }
    else    // m_ptCurrent.Y <= 0
    {
        if (ptEnd.Y > 0)
        {
            // We have crossed the x axis going up
            if (ptEnd.X * m_ptCurrent.Y - m_ptCurrent.X * ptEnd.Y >= 0)
            {
                // The crossing was on the positive side 
                m_iWinding++;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//              Implementation of CMILBezierFlattener
//+-----------------------------------------------------------------------------
//
//  Member:   CMILBezierFlattener::CMILBezierFlattener
//
//  Synopsis: Constructor from individual points, no trimming
//
//------------------------------------------------------------------------------
CMILBezierFlattener::CMILBezierFlattener(
    __in_ecount(1) const GpPointR &ptFirst,
        // First point (transformed)
    __in_ecount(1) const GpPointR &ptControl1,
        // First control point
    __in_ecount(1) const GpPointR &ptControl2,
        // Second control point
    __in_ecount(1) const GpPointR &ptEnd,
        // Last point
    __in_ecount_opt(1) CFlatteningSink *pSink,
        // Flattening sink
    __in_ecount(1) const CMILMatrix &matrix)
        // Transformation matrix
    : CBezierFlattener(pSink, DEFAULT_FLATTENING_TOLERANCE)
{
     m_ptB[0] = ptFirst;
     TransformPoint(matrix, ptControl1, OUT m_ptB[1]);
     TransformPoint(matrix, ptControl2, OUT m_ptB[2]);
     TransformPoint(matrix, ptEnd, OUT m_ptB[3]);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CMILBezierFlattener::SetPoints
//
//  Synopsis:
//      Set the coefficients for a possibly transformed and possibly trimmed
//      curve
//
//  Notes:
//      This method is geared towards traversing a path with a transformation.
//      The first point is equal to the last point of the previous segment,
//      which has already been transformed; that is why it is entered
//      separately.
//
//      The curve defined by the input points is a parametric mapping from the
//      interval [0,1]. The input arguments rStart and rEnd allow the caller to
//      specify setting the coefficients to represent a trimmed portion of the
//      of the original curve.
//
//------------------------------------------------------------------------------
void
CMILBezierFlattener::SetPoints(
    double rStart,
        // Start parameter
    double rEnd,
        // End parameter
    __in_ecount(1) const GpPointR  &ptFirst,
        // First point (transformed)
    __in_ecount(3) const MilPoint2F *pt,
        // The last 3 points (raw)
    __in_ecount_opt(1) const CMILMatrix  *pMatrix)
        // Transformation matrix (NULL OK)
{
    // The caller should not be asking for trimming outside [0,1].
    // Ignore NaNs
    Assert(!(0 > rStart)); 
    Assert(!(rStart > rEnd));
    Assert(!(rEnd > 1));

    m_ptB[0] = ptFirst;
    if (pMatrix)
    {
        TransformPoints(*pMatrix, 3, pt, m_ptB + 1);
    }
    else
    {
        for (int i = 1;  i <= 3;  i++)
        {
            m_ptB[i] = pt[i - 1];
        }
    }

    // Trimming = computing Bezier points for a curve that represents a portion
    // of the curve defined by the input points.
    if (rEnd <= rStart + FUZZ)
    {
        // The trimmed curve degenerates to a point
        GpPointR pt;
        GetPoint(rStart, pt);
        m_ptB[0] = m_ptB[1] = m_ptB[2] = m_ptB[3] = pt;
    }
    else
    {
        if (rStart > 0)
        {
            TrimToStartAt(rStart);
        }
        if (rEnd < 1)
        {
        // If rStart > 0 then the curve has been trimmed, but the Bezier points represent a
        // curve with parameter domain [0,1], oblivious to that trimming.  So we need to
        // adjust the second trimming parameter to reflect the first trimming. For example, 
        // supposed rStart = 0.2 and rEnd = 0.6. After trimming 0.2 from the start, we want the
        // second trim to leave us with [0.2, 0.6].  The size of this domain is 0.4, which is
        // 0.5 of 0.8 - the size remaining after the first trim.  We get that with 
        // (0.6-0.2)/(1-0.2). In general, the new trim parameter is 
        // (rEnd - rStart) / (1 - rStart).

            if (rStart > 0)
            {
                // Ignore NaNs
                Assert(!(FUZZ >= 1 - rStart));  // Since rStart + FUZZ < rEnd <= 1
                rEnd = (rEnd - rStart) / (1 - rStart);
            }
            TrimToEndAt(rEnd);
        }
    }
}

#ifdef DBG
//////////////////////////////////////////////////////////////////////////
//
//              Implementation of CFigureDumper
//+-----------------------------------------------------------------------
//
//  Member:    CFigureDumper::DoLine
//
//  Synopsis:  Dump line segment - CFigureTask override
//
//  Returns:   Always false
//
//------------------------------------------------------------------------
HRESULT
CFigureDumper::DoLine(
    __in_ecount(1) const MilPoint2F &pt
        // The line's end point
    )
{
    MILDebugOutput(L"Line to (%f, %f)\n", pt.X, pt.Y); 
    
    return S_OK;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureDumper::DoBezier
//
//  Synopsis:
//      Dump a Bezier segment - CFigureTask override
//
//  Returns:
//      Always false.
//
//------------------------------------------------------------------------------
HRESULT 
CFigureDumper::DoBezier(
    __in_ecount(3) const MilPoint2F *pt
        // The curve's 3 last Bezier points
    )
{
    MILDebugOutput(
        L"Bezier to (%f, %f), (%f, %f), (%f, %f)\n", 
         pt[0].X, pt[0].Y,
         pt[1].X, pt[1].Y,
         pt[2].X, pt[2].Y);

    return S_OK;
}
#endif // def DBG
//////////////////////////////////////////////////////////////////////////
//
//              Implementation of CFigureTask

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureTask::TraverseForward
//
//  Synopsis:
//      Traverse the figure forward
//
//------------------------------------------------------------------------------
HRESULT
CFigureTask::TraverseForward(
    __in_ecount(1) const IFigureData &figure
        // The traversed figure
    )
{
    HRESULT hr = S_OK;
    const MilPoint2F *pPt;
    BYTE bType;
    m_fAborted = false;

    if (!figure.SetToFirstSegment())
        goto Cleanup;
    
    do 
    {
        figure.GetCurrentSegment(bType, pPt);
        if (MilCoreSeg::TypeLine == bType)
        {          
            IFC(DoLine(*pPt));
        }
        else
        {
            Assert(MilCoreSeg::TypeBezier == bType);
            IFC(DoBezier(pPt));
        }
    } 
    while (!m_fAborted  &&  figure.SetToNextSegment());

Cleanup:
    RRETURN(hr);
}

#ifdef LINE_SHAPES_ENABLED
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureTask::TraverseBackward
//
//  Synopsis:
//      Traverse the figure backward
//
//  Notes:
//      In addition to traversing the segments backward, each segment is
//      processed in reverse.
//
//------------------------------------------------------------------------------
HRESULT
CFigureTask::TraverseBackward(
    __in_ecount(1) const IFigureData &figure
        // The traversed figure
    )
{
    HRESULT hr = S_OK;
    const MilPoint2F *pPt;
    BYTE bType;
    MilPoint2F P[3];
    m_fAborted = false;
    
    if (!figure.SetToLastSegment())
        goto Cleanup;

    do
    {
        figure.GetCurrentSegment(bType, pPt);
        P[2] = figure.GetCurrentSegmentStart();

        if (MilCoreSeg::TypeLine == bType)
        {
            IFC(DoLine(P[2]));
        }
        else
        {
            // Reverse the segment direction
            P[0] = pPt[1];
            P[1] = pPt[0];
            // P[2] has been set above

            IFC(DoBezier(P));
        }
    }
    while (!m_fAborted  &&  figure.SetToPreviousSegment());

Cleanup:
    RRETURN(hr);
}
#endif // LINE_SHAPES_ENABLED

