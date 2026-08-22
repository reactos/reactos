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
//      Implements the stroking of a figure
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CRail, MILRender, "CRail");

#ifdef DBG
bool g_fWidenTrace = false;
#define WIDEN_TRACE(x) if(g_fWidenTrace) OutputDebugString(x);
#else
#define WIDEN_TRACE(x)
#endif

//+-----------------------------------------------------------------------------
//
//  Class:
//      CMatrix22
//
//  Synopsis:
//      Implements a 2x2 matrix class 
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Set from raw data
//
//------------------------------------------------------------------------------
CMatrix22::CMatrix22(
    __in_ecount(1) const CMatrix22 &other) // In The matrix to copy

{
    m_rM11 = other.m_rM11;
    m_rM12 = other.m_rM12;
    m_rM21 = other.m_rM21;
    m_rM22 = other.m_rM22;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Reset to identity
//
//------------------------------------------------------------------------------
void
CMatrix22::Reset()
{
    m_rM11 = m_rM22 = 1;
    m_rM12 = m_rM21 = 0;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Set from raw data
//
//------------------------------------------------------------------------------
void
CMatrix22::Set(
    GpReal rM11, // In: The value to set for M11
    GpReal rM12, // In: The value to set for M12
    GpReal rM21, // In: The value to set for M21
    GpReal rM22) // In: The value to set for M22
{
    m_rM11 = rM11;
    m_rM12 = rM12;
    m_rM21 = rM21;
    m_rM22 = rM22;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Prepend a CMILMatrix to this matrix. The translation portion is ignored.
//
//------------------------------------------------------------------------------
void
CMatrix22::Prepend(
    __in_ecount_opt(1) const CMILMatrix *pMatrix
        // In: The matrix to prepend
    )
{
    if (pMatrix)
    {
        REAL K[6] = 
        {
            pMatrix->GetM11(),
            pMatrix->GetM12(),
            pMatrix->GetM21(),
            pMatrix->GetM22(),
            pMatrix->GetDx(),
            pMatrix->GetDy()
        };

        GpReal M1 = m_rM11;
        GpReal M2 = m_rM12;
        m_rM11 = M1 * K[0] + M2 * K[2];
        m_rM12 = M1 * K[1] + M2 * K[3];
 
        M1 = m_rM21;
        M2 = m_rM22;
        m_rM21 = M1 * K[0] + M2 * K[2];
        m_rM22 = M1 * K[1] + M2 * K[3];
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMatrix22::Finalize
//
//  Synopsis:
//      Get the inverse of this matrix, possibly adjusting this matrix
//
//  Returns:
//      true if |determinant| >= threshold and the matrix is invertible
//
//  Notes:
//      This is not a const method!  If this matrix represents a flipping
//      transformation, where left is switched with right, this method will
//      prepend flip to it.
//
//      We remove flips because they will switch the offset, from left to right,
//      confusing the algorithm. Since the matrix will be applied to the pen
//      shape, which is a circle, it doesn't affect the final pen's shape, due
//      to the perfect symmetry of circles.  This will not work with other pen
//      shapes!
//
//      We are called after verifying that at least one entry of the matrix M is 
//      greater than the stroke emptiness threshold. That means that |V*M| > threshold*|V|
//      for some vector V.  The magification factor of M is <= its max(abs(eigenvalues)),
//      and det(M) is the product of its eigenvalues.  So here if 
//      |det(M)| <= (threshold squared) then |W*M| <= threshold*|W| for some vector W,
//      and the pen is too thin in that direction.
//
//------------------------------------------------------------------------------
bool
CMatrix22::Finalize(
    GpReal rEmptyThresholdSquared, 
        // Lower bound for |determinant(this)|
    __out_ecount(1) CMatrix22 &oInverse)
        // The inverse of this matrix
{
    GpReal rDet = m_rM11 * m_rM22 - m_rM12 * m_rM21;
    
    // Ignore NaNs
    Assert(!(rEmptyThresholdSquared <= 0));
    
    bool fEmpty = (fabs(rDet) < rEmptyThresholdSquared);
    if (fEmpty)
        goto exit;

    // Make sure the matrix does not flip
    if (rDet < 0)
    {
        // Prepend an X flip
        m_rM11 = -m_rM11;
        m_rM12 = -m_rM12;
        rDet = -rDet;
    }

    // Now set the inverse
    rDet = 1.0 / rDet;
    oInverse.m_rM11 = m_rM22 * rDet;
    oInverse.m_rM12 = -m_rM12 * rDet;
    oInverse.m_rM21 = -m_rM21 * rDet;
    oInverse.m_rM22 = m_rM11 * rDet;

exit:
    return !fEmpty;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Does this matrix preserve circles?
//
//      It does if M11 = M22 and M12 = -M21 within tolerance. As a side effect
//      (because it is needed internally) this method returns a bound on the
//      squared scale factor of this matrix.  If isotropic, the bound is exact -
//      it is the uniform scale factor squared.  Otherwise, it is the sum of the
//      squares of the matrix entries.
//
//------------------------------------------------------------------------------
bool
CMatrix22::IsIsotropic(
    __out_ecount(1) GpReal &rSqMax
        // Out: Bound on the squared scale factor of this matrix
) const
{
    // Exact test rather than with fuzz because it is cheaper, and a false
    // negative may slow us down but will still produce correct results.
    bool fIsotropic = ((m_rM11 == m_rM22) && (m_rM12 == -m_rM21));

    rSqMax = m_rM11 * m_rM11 + m_rM12 * m_rM12;
    if (!fIsotropic)
    {
        rSqMax += (m_rM21 * m_rM21 + m_rM22 * m_rM22);
    }

    return fIsotropic;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Invert the matrix in place
//
//------------------------------------------------------------------------------
HRESULT
CMatrix22::Invert()
{
    GpReal rDet = m_rM11 * m_rM22 - m_rM12 * m_rM21;

    if (rDet == 0.0)
    {
        RRETURN(WGXERR_BADNUMBER);
    }

    // Set the inverse
    rDet = 1 / rDet;

    if (!_finite(rDet))
    {
        RRETURN(WGXERR_BADNUMBER);
    }
    
    GpReal rTemp = m_rM22 * rDet;
    m_rM12 = -m_rM12 * rDet;
    m_rM21 = -m_rM21 * rDet;
    m_rM22 = m_rM11 * rDet;
    m_rM11 = rTemp;

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Compute the coefficients of the pre-transform length
//
//      Given a transformed vector V under the transformation M, we need to
//      compute the length of the pre-transformed vector, which is VN where N is
//      the inverse of M.  The length of VN will be sqrt((VN)(VN)'), where '
//      stands for transpose.  But (VN)(VN) = V(NN')V'.
//
//        if N = (a b) then  NN' = (p q)
//               (c d)             (q r)
//
//      where p = a^2+b^2,  q = ac+bd, and r = c^2+d^2.
//
//      If V = (x,y) then V(NN')V' = px^2 + 2qxy + ry^2, a quadratic function of
//      x and y.  This method computes the coefficients of this function.
//
//------------------------------------------------------------------------------
HRESULT
CMatrix22::GetInverseQuadratic(
    __out_ecount(1) GpReal &rCxx,
        // Out: Coefficient of x*x
    __out_ecount(1) GpReal &rCxy,
        // Out: Coefficient of x*y
    __out_ecount(1) GpReal &rCyy
        // Out: Coefficient of y*y
    )
{
    HRESULT hr = S_OK;

    IFC(Invert());

    rCxx = m_rM11 * m_rM11 + m_rM12 * m_rM12; 
    rCxy = 2 * (m_rM11 * m_rM21 + m_rM12 * m_rM22);
    rCyy = m_rM21 * m_rM21 + m_rM22 * m_rM22; 

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Multiply a column vector with the transpose of the matrix
//
//------------------------------------------------------------------------------
void
CMatrix22::TransformColumn(
    __inout_ecount(1) GpPointR &P
        // In/out: A vector, muliplied in place
    ) const
{
    GpReal r = m_rM11 * P.X + m_rM12 * P.Y;
    P.Y = m_rM21 * P.X + m_rM22 * P.Y;
    P.X = r;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Transform in place
//
//------------------------------------------------------------------------------
void
CMatrix22::Transform(
    __inout_ecount(1) GpPointR &P
    ) const
{
    GpReal r = m_rM11 * P.X + m_rM21 * P.Y;
    P.Y = m_rM12 * P.X + m_rM22 * P.Y;
    P.X = r;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Prepend an X flip
//
//------------------------------------------------------------------------------
void
CMatrix22::PreFlipX()
{
    m_rM11 = - m_rM11;
    m_rM12 = - m_rM12;
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CWidener
//
//  Synopsis:
//      Generates a figure that approximates the stroke of a pen along a
//      figure.
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      CWidener working constructor.
//
//------------------------------------------------------------------------------
CWidener::CWidener(
    GpReal rTolerance 
        // In: Approximation tolerance
    )
    : m_rTolerance(rTolerance), 
    m_eStartCap(MilPenCap::Flat), 
    m_eEndCap(MilPenCap::Flat), 
    m_eLineJoin(MilLineJoin::Round),
    m_pTarget(&m_pen),
    m_oLine(rTolerance),
    m_oCubic(rTolerance),
    m_dasher(&m_pen)
{
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Construct and set the internal pen for this widening.
//
//      This private method constructs an internal pen of the class that is
//      determined by the geometry of the stroking pen.  This internal pen
//      captures the rendering transformation, and it is hooked up the pen to
//      the sink into which it will generate.
//
//------------------------------------------------------------------------------
HRESULT
CWidener::Initialize(
    __in_ecount(1) const CPlainPen &pen,
        // The stroking pen
    __in_ecount(1) CWideningSink *pSink,
        // The widening sink
    __in_ecount_opt(1) const CMILMatrix *pMatrix,
        // Render transform (NULL OK)
    __in_ecount_opt(1) const CMILSurfaceRect *prcViewable,
        // The viewable region (NULL OK)
    __out_ecount(1) bool &fEmpty)
        // The pen is empty if true
{
    HRESULT hr = S_OK;

    CMilRectF rcViewableInflated;
    const MilRectF *prcViewableInflated = NULL;

    m_eStartCap = pen.GetStartCap();
    m_eEndCap = pen.GetEndCap();
    m_eDashCap = pen.GetDashCap();
    m_eLineJoin = pen.GetJoin();
    
    // Should have been detected before calling us:
    Assert(!pen.IsEmpty()); 
    Assert(!prcViewable || !prcViewable->IsEmpty());
    
    if (pMatrix && !pMatrix->IsIdentity())
    {
        m_pMatrix = pMatrix;
    }
    else
    {
        m_pMatrix = NULL;
    }

#ifdef COMPOUND_PEN_IMPLEMENTED    
    // Construct the appropriate pen
    if (pen.IsCompound())
    {
        // Allocate a new compound pen - but for now:
        IFC(E_NOTIMPL); 
    }
#endif // COMPOUND_PEN_IMPLEMENTED
    
    if (prcViewable)
    {
        IFC(GetViewableInflated(
            prcViewable,
            pen,
            m_pMatrix,
            &rcViewableInflated));

        prcViewableInflated = &rcViewableInflated;
    }
    
    fEmpty = !m_pen.Initialize(
                pen.GetGeometry(),
                m_pMatrix,
                m_rTolerance,
                prcViewableInflated,
                pSink);

    if (MilDashStyle::Solid == pen.GetDashStyle())
    {
        SetTarget(&m_pen);
    }
    else
    {
        // Hook in the dasher
        SetTarget(&m_dasher);
        
        //
        // Future Consideration:  Right now we're just using the clip for
        // dashes, but we could use it for segments, too.
        //
        IFC(m_dasher.Initialize(pen, m_pMatrix, prcViewableInflated));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CWidener::SetSegment
//
//  Synopsis:
//      Set for widening the current segment
//
//  Returns:
//      true if this segment is not empty
//
//  Notes:
//      The transformed first point is taken from the previous segment
//
//------------------------------------------------------------------------------
bool
CWidener::SetSegmentForWidening(
    __in_ecount(1) const IFigureData &oFigure,
        // The figure
    __inout_ecount(1) GpPointR &ptFirst,
        // First point, transformed, possibly modified here
    __in_ecount_opt(1) const CMILMatrix *pMatrix
        // Transformation matrix (NULL OK)
    ) const
{
    const MilPoint2F *pt;
    BYTE bType;
    double rTrim = 1.0;
    bool fEmpty = false;

    if (oFigure.GetCurrentSegment(bType, pt))
    {
        //
        // This is the last segment, it may be trimmed at the end for a line
        // shape.
        //
        rTrim = m_rEndTrim;
        fEmpty = m_rEndTrim <= m_rStartTrim;
    }

    if (MilCoreSeg::TypeBezier == bType)
    {
        m_oCubic.Set(m_rStartTrim, rTrim, ptFirst, pt, pMatrix);
        m_pSegment = &m_oCubic;
    }
    else
    {
        m_oLine.Set(m_rStartTrim, rTrim, ptFirst, *pt, pMatrix);
        m_pSegment = &m_oLine;
    }

    return !fEmpty;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Widen one figure (subpath)
//
//------------------------------------------------------------------------------
HRESULT
CWidener::Widen(
    __in_ecount(1) const IFigureData &oFigure,
        // The figure
    __in_ecount_opt(1) CStartMarker *pStartMarker,
        // Start shape marker (NULL OK)
    __in_ecount_opt(1) CEndMarker *pEndMarker
        // End shape marker (NULL OK)
    ) const
{
    HRESULT hr = S_OK;
    
    if (oFigure.IsEmpty())
    {
        goto Cleanup;
    }

    //
    // A stop is a place to terminate traversal.  None should be set outside
    // widening.
    //
    Assert(!oFigure.IsStopSet());

    //
    // m_fShouldPenBeDown indicates if we are at a segment that is SUPPOSED to
    // be widened.
    //
    // m_fIsPenDown indicates if the current widening stretch has
    // ACTUALLY started, i.e. we're not in a gap and the current segment is
    // non-degenerate. 
    //
    m_fIsPenDown = m_fSkippedFirst = false;
    m_fShouldPenBeDown = oFigure.HasNoSegments();

    m_rStartTrim = 0;
    m_rEndTrim = 1;
    
    if (oFigure.IsClosed())
    {
        IFC(WidenClosedFigure(oFigure));
    }
    else
    {
        IFC(WidenOpenFigure(oFigure, pStartMarker, pEndMarker));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Constructor:
//      CWidener::WidenOpenFigure
//
//  Synopsis:
//      Widen an open figure.
//
//------------------------------------------------------------------------------
HRESULT
CWidener::WidenOpenFigure(
    __in_ecount(1) const IFigureData &oFigure,
        // The figure
    __in_ecount_opt(1) CStartMarker *pStartMarker,
        // Start shape marker (NULL OK)
    __in_ecount_opt(1) CEndMarker *pEndMarker
        // End shape marker (NULL OK)
   ) const
{
    HRESULT hr = S_OK;
    bool fTrimmedAway = false;

    m_fClosed = false;

    // Cap may switch to dash cap if we have gaps, but for now ---
    m_eCap = m_eStartCap;

    m_fNeedToRecordStart = false;


    
#ifdef LINE_SHAPES_ENABLED
    //
    // Process markers, possibly trimming the figure.  This may set the figure
    // in a state of stopping before its end.  We will reset it
    //
    if (pEndMarker)
    {
        IFC(pEndMarker->Process(oFigure, fTrimmedAway, m_rEndTrim));
    }

    if (pStartMarker)
    {
        IFC(pStartMarker->Process(oFigure, fTrimmedAway, m_rStartTrim));
        //
        // Processing for start shape leaves the figure's traversal state at
        // the segment where it is trimmed for start shape, which is where
        // widening should start.  Otherwise we need to set it manually to
        // figure start:
        //
    }
    else 
#endif // LINE_SHAPES_ENABLED
        
    if (!oFigure.SetToFirstSegment())
    {
        goto Cleanup;
    } 

    // Is there anything left after trimming?
    if (fTrimmedAway)
    {
        // The figure has been trimmed away entirely by the line shapes
        goto Cleanup;
    }

    if (oFigure.IsAtAGap())
    {
        //
        // First segment is a gap, so when we really do start, we want to start
        // with the dash cap, not the start cap.
        //

        m_eCap = m_eDashCap;
    }

    do
    {
        if (oFigure.IsAtAGap())
        {
            IFC(DoGap(oFigure));
            m_fSmoothJoin = false;
        }
        else    // This segment is not a gap
        {
            IFC(DoSegment(oFigure));
            m_fSmoothJoin = oFigure.IsAtASmoothJoin();
        }
    }
    while (oFigure.SetToNextSegment()  &&  !m_pTarget->Aborted());

    // Wrap up
    if (m_fShouldPenBeDown)
    {
        IFC(m_pTarget->EndStrokeOpen(m_fIsPenDown, m_pt, m_vecIn, m_eEndCap, m_eCap));
    }

Cleanup:
    // A stop may have been set when processed for line shapes
    oFigure.ResetStop();

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Constructor:
//      CWidener::WidenClosedFigure
//
//  Synopsis:
//      Widen a closed figure.
//
//------------------------------------------------------------------------------
HRESULT
CWidener::WidenClosedFigure(
    __in_ecount(1) const IFigureData &oFigure
        // The figure
    ) const
{
    HRESULT hr = S_OK;
 
    MilPenCap::Enum eStartCap = MilPenCap::Flat;
    MilPenCap::Enum eEndCap = MilPenCap::Flat;

    m_fClosed = true;
    bool fAbut = false;
    m_fNeedToRecordStart = true;

    if (!oFigure.SetToFirstSegment())
    {
        goto Cleanup;
    }

    if(oFigure.HasGaps())
    {
        // Closed figure with gaps, handled as an open figure
        if (oFigure.IsAtAGap())
        {
            // The first segment is a gap.
            // The start and end cap will be dash caps
            eStartCap = eEndCap = m_eDashCap;
        }
        else
        {
            // The first segment is be a continuation of the last segment,
            // cap them with abutting flat caps.
            eStartCap = eEndCap = MilPenCap::Flat;
            fAbut = true;
        }
        m_fClosed = false;
    }

    //
    // Initially eStartCap may be m_eStartCap, but it may have already been set
    // to dash cap or a flat above, if the figure has gaps.  We set the current
    // m_eCap type to that now.  and will restore it when we are done.
    //
    m_eCap = eStartCap;
    do
    {
        if (oFigure.IsAtAGap())
        {
            IFC(DoGap(oFigure));
            m_fSmoothJoin = false;
        }
        else    // This segment is not a gap
        {
            IFC(DoSegment(oFigure));
            m_fSmoothJoin = oFigure.IsAtASmoothJoin();
        }
    } 
    while (oFigure.SetToNextSegment()  &&  !m_pTarget->Aborted());

    // Wrap up
    if (m_fShouldPenBeDown)
    {
        // The last segment is not a gap
        if (m_fClosed)
        {
            // The figure has no gaps, so it is handled as one closed stroke.
            if (m_fIsPenDown)
            {
                //
                // The figure is handled as a closed stroke, and the widening
                // was started.  This is the most common scenario.
                //
                IFC(m_pTarget->DoCorner(
                    m_ptStart,
                    m_vecIn, 
                    m_vecStart, 
                    m_eLineJoin, 
                    m_fSkipped  || m_fSkippedFirst,
                    m_fSmoothJoin,
                    true)); // ==>This is a figure-closing corner
                
                IFC(m_pTarget->EndStrokeClosed(m_ptStart, m_vecStart));
            }
            else
            {
                //
                // No gaps, handled as a closed stroke, but widening never
                // started. This means that the figure is degenerate, so just
                // draw a single point with round caps.  Achieved by ending it
                // as an open stroke that never started.
                //

                IFC(m_pTarget->EndStrokeOpen(
                        false,               // Never started
                        m_pt,                // Current point 
                        m_vecIn,             // To be ignored, since not started
                        MilPenCap::Round,    // End cap 
                        MilPenCap::Round));  // Start cap
            }
        }
        else
        {
            // The figure has gaps, so it is handled one or more open strokes.
            if (fAbut)
            {
                //
                // The first segment is not a gap, so end and start abut with
                // flat caps
                //
                if (m_fIsPenDown)
                {
                    //
                    // Abutting end with start while a valid end-segment is in
                    // progress.  Do the corner between the first and that last
                    // segment.
                    //
                    IFC(m_pTarget->DoCorner(m_pt, 
                            m_vecIn,
                            m_vecStart,
                            m_eLineJoin,
                            m_fSkipped  || m_fSkippedFirst,
                            m_fSmoothJoin,
                            true)); // ==> This is a figure-closing corner
                }
                else
                {
                    //
                    // Abutting end with start but the end-segment was never
                    // started.  So start one with a dash cap at the starting
                    // point and direction
                    //
                    IFC(m_pTarget->StartFigure(m_ptStart, m_vecStart, false, m_eDashCap));
                    m_fIsPenDown = true;
                }

                //
                // Now the last segment matches the starting point and
                // direction, so cap it with a flat cap to abut the starting
                // segment.
                //
                IFC(m_pTarget->EndStrokeClosed(m_pt, m_vecStart));
            }
            else
            {
                //
                // Handled as one or more open strokes but no abbutting,
                // because the first segment is a gap. Cap the end of the
                // stroke with a dash cap
                //
                hr= THR(m_pTarget->EndStrokeOpen(m_fIsPenDown, m_pt, m_vecIn, eEndCap, m_eDashCap));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Constructor:
//      CWidener::DoGap
//
//  Synopsis:
//      Process a segment as a gap
//
//  Notes:
//      If the previous segment was not a gap then we need to cap it with a dash
//      cap.
//
//------------------------------------------------------------------------------
HRESULT
CWidener::DoGap(
    __in_ecount(1) const IFigureData &oFigure
        // The figure
    ) const
{
    HRESULT hr = S_OK;

    if (m_fShouldPenBeDown)
    {
        // The previous segment was not a gap, so cap it

        // Interior segments start and end with a dash cap
        m_eCap = m_eDashCap;  
        IFC(m_pTarget->EndStrokeOpen(m_fIsPenDown, m_pt, m_vecIn, m_eCap));
        m_fShouldPenBeDown = m_fIsPenDown = false;
    }
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Constructor:
//      CWidener::DoSegment
//
//  Synopsis:
//      Process a widened segment
//
//  Notes:
//      If the previous segment was not a gap then we need to cap it with a dash
//      cap.
//
//------------------------------------------------------------------------------
HRESULT
CWidener::DoSegment(
    __in_ecount(1) const IFigureData &oFigure
        // The figure
    ) const
{
    HRESULT hr = S_OK;

    WIDEN_TRACE(L"CWidener::DoSegment\n");

    if (!m_fShouldPenBeDown) // Figure start or after a gap - get the initial point
    {
        m_pt = GpPointR(oFigure.GetCurrentSegmentStart(), m_pMatrix);
        m_fShouldPenBeDown = true;
    }

    //
    // Set up a line or Bezier widening segment object
    //
    // Possible side effect: m_pt may be modified if the segment is trimmed for
    // line shape.
    //
    if (!SetSegmentForWidening(oFigure, m_pt, m_pMatrix))
        goto Cleanup;

    if (!SUCCEEDED(m_pSegment->GetFirstTangent(m_vecOut)))
    {
        // This segment is degenerate, skip it
        m_fSkipped = true;
        if (!m_fIsPenDown)
            m_fSkippedFirst = true;
        goto Cleanup;
    }

    if (m_fIsPenDown)
    {
        // This is not the first segment; do the corner before widening it
        IFC(m_pTarget->DoCorner(m_pt, 
                m_vecIn, 
                m_vecOut, 
                m_eLineJoin, 
                m_fSkipped, 
                m_fSmoothJoin,
                false));    // ==> This is not a figure-closing corner
    }
    else
    {
        //
        // This is the beginning of a stroke - either at figure start or after
        // a gap - so start the figure before widening it.
        //
        IFC(m_pTarget->StartFigure(m_pt, m_vecOut, m_fClosed, m_eCap));

        if (m_fNeedToRecordStart)
        {
            // For closing the figure later, if closed
            m_ptStart = m_pt;
            m_vecStart = m_vecOut;

            //
            // We want to record only the start of the first segment, not the
            // start of a new stroke after a gap, so we want to record it only
            // once.  If the figure starts with a gap then we have recorded the
            // the wrong thing, but in that case we will not close the figure,
            // so we'll never need this point and vector.
            //
            m_fNeedToRecordStart = false;
        }

        m_fIsPenDown = true;
        m_rStartTrim = 0;   // In case the start was trimmed for a line shape
    }

    IFC(m_pSegment->Widen(m_pt, m_vecIn)); // This updates vecIn
    m_fSkipped = false;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Constructor:
//      CWidener::SetForLineShape
//
//  Synopsis:
//      Set this widener for widening a line shape
//
//  Notes:
//      We may inherit the widening pen from the widener used for the stroke to
//      which we're attaching this shape.  We'll modify the pen to be simple
//      (not compound), and ignore any dasher.
//
//      If the line shape overrides the pen then a new widening pen will be
//      computed from the overriding pen-geometry.
//
//------------------------------------------------------------------------------
HRESULT
CWidener::SetForLineShape(
    __in_ecount(1) const CWidener &other,
        // The widener used for of the path to which the line shape is attached
    __in_ecount(1) const CLineShape &shape,
        // The line shape we're attaching
    __in_ecount(1) CWideningSink *pSink,
        // The widening sink
    __out_ecount(1) bool &fEmpty)
        // =true if the pen is empty
{
#ifdef LINE_SHAPES_ENABLED
    HRESULT hr = S_OK;

    if (shape.OverridesThePen())
    {
        const CPenGeometry &geom = shape.GetPenGeometry();

        m_eStartCap = geom.GetStartCap(); 
        m_eEndCap = geom.GetEndCap();
        m_eLineJoin = geom.GetJoin();
        fEmpty = !m_pen.Initialize(geom, other.m_pMatrix, other.m_rTolerance, NULL, pSink);
    }
    else
    {
        m_eStartCap = other.m_eStartCap; 
        m_eEndCap = other.m_eEndCap;
        m_eLineJoin = other.m_eLineJoin;
        m_pen.SetFrom(other.m_pen, pSink);
        fEmpty = false;
    }

    SetTarget(&m_pen);

    RRETURN(hr);
#else
    Assert(false);
    RRETURN(E_NOTIMPL);
#endif // LINE_SHAPES_ENABLED
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CWidener::WidenLineShape
//
//  Synopsis:
//      Widen a line shape
//
//  Notes:
//      A line shape strokes are widened differently:
//      * There is a transformation that positions the shape at the tip of
//        the line
//      * There are no line shapes, no dashes and no compound line
//
//------------------------------------------------------------------------------
HRESULT
CWidener::WidenLineShape(
    __in_ecount(1) const CShape &shape,
        // The widened shape
    __in_ecount_opt(1) const CMILMatrix *pMatrix)
        // The positioning matrix
{
    HRESULT hr = S_OK;
    UINT i;

    m_pMatrix = pMatrix;

    for (i = 0;  i < shape.GetFigureCount();  i++)
    {
        IFC(Widen(shape.GetFigure(i), NULL, NULL));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CPen
//  
//  Synopsis:
//      Implements an (undashed) pen.
//

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CPen::CPen()
    : m_eLineJoin(MilLineJoin::Round),
      m_rNominalMiterLimit(1),
      m_rRadius(1),
      m_rRadSquared(1),
      m_rMiterLimit(1),
      m_rMiterLimitSquared(1),
      m_rRefinementThreshold(1),
      m_fCircular(false)
{
}    

//+-----------------------------------------------------------------------------
//
//  Member:
//      CPen::Set
//
//  Synopsis:
//      Set the widening pen properties
//
//  Returns:
//      false if the pen is effectively empty
//
//------------------------------------------------------------------------------
bool
CPen::Set(
    __in_ecount(1) const CPenGeometry  &geom,
        // The pen's geometry information
    __in_ecount_opt(1)const CMILMatrix *pMatrix,
        // W to D transformation matrix (NULL OK)
    GpReal rTolerance,
        // Approximation tolerance
    __in_ecount_opt(1) const MilRectF *prcViewableInflated
        // Viewable region (inflated by stroke properties)
    )
{
    // Ignore NaNs
    Assert(!(rTolerance <= 0));

    bool fEmpty = !SetPenShape(geom, pMatrix, rTolerance);
    if (fEmpty)
        goto exit;

    // Store the world to device matrix
    m_oWToDMatrix.Reset();
    m_oWToDMatrix.Prepend(pMatrix);
    m_eLineJoin = geom.GetJoin();

    m_rNominalMiterLimit = geom.GetMiterLimit();
    if (m_rNominalMiterLimit < 1)
        m_rNominalMiterLimit = 1;

    m_rMiterLimit = m_rNominalMiterLimit * m_rRadius;
    m_rMiterLimitSquared = m_rMiterLimit * m_rMiterLimit;

    if (prcViewableInflated != NULL)
    {
        m_rcViewableInflated = *prcViewableInflated;
        m_fViewableSpecified = true;
    }
    else
    {
        m_fViewableSpecified = false;
    }

exit:
    return !fEmpty;
}    

//+-----------------------------------------------------------------------------
//
//  Member:
//      CPen::Copy
//
//  Synopsis:
//      Copy from another pen
//
//------------------------------------------------------------------------------
void
CPen::Copy(
    __in_ecount(1) const CPen &pen
        // A pen to copy basic properties from
    )
{
    m_eLineJoin = pen.m_eLineJoin;
    m_oMatrix = pen.m_oMatrix;
    m_oInverse = pen.m_oInverse;
    m_oMatrix = pen.m_oWToDMatrix;
    m_rRadius = pen.m_rRadius;
    m_rRadSquared = pen.m_rRadSquared;
    m_rNominalMiterLimit = pen.m_rNominalMiterLimit;
    m_rMiterLimit = pen.m_rMiterLimit;
    m_rMiterLimitSquared = pen.m_rMiterLimitSquared;
    m_rRefinementThreshold = pen.m_rRefinementThreshold;
    m_fCircular = pen.m_fCircular;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CPen::SetPenShape
//
//  Synopsis:
//      Set the pen's shape parameters. Private method.
//
//  Returns:
//      False if the pen is effectively empty (relative to the approximation
//      tolerance).
//
//  Notes:
//      The pen is circular if its width and height are equal.  But the
//      presence of a render transform may change circular to non-circular and
//      vice versa.
//
//      If the pen is deemed circular then we model it as a circle of the given
//      radius with an identity transformation.  If non-circular, we model it as
//      a circle of radius 1 mapped by the transformation.
//
//------------------------------------------------------------------------------
bool
CPen::SetPenShape(
    __in_ecount(1) const CPenGeometry &geom,
        // The pen's geometry information
    __in_ecount_opt(1) const CMILMatrix *pMatrix,
        // Render transform (NULL OK)
    GpReal rTolerance
        // Approximation tolerance
    )
{
    GpReal rFactor = 1;
    GpReal w = geom.GetWidth() / 2;
    GpReal h = geom.GetHeight() / 2;
    GpReal rMaxRadiusBound = max(w,h);
    bool fEmpty = false;

    GpReal rEmptyThreshold = rTolerance * EMPTY_PEN_FACTOR;
    GpReal rEmptyThresholdSquared = rEmptyThreshold * rEmptyThreshold;

    if (0 == geom.GetAngle()) // Exact test is OK, this is just a shortcut
    {
        m_oMatrix.Set(w, 0, 0, h);
    }
    else
    {
        GpReal c = cos(geom.GetAngle());
        GpReal s = sin(geom.GetAngle());
        m_oMatrix.Set(w * c,  -w * s, 
                      h * s,  h * c); 
    }

    if (pMatrix)
    {
        rMaxRadiusBound *= pMatrix->GetMaxFactor();

        // Fold the rendering transformation into the pen's matrix
        m_oMatrix.Prepend(pMatrix);
        
        m_fCircular = m_oMatrix.IsIsotropic(OUT rFactor);

        fEmpty = rFactor < rEmptyThresholdSquared;
        if (fEmpty)
        {
            // All the matrix entries are small
            goto exit;
        }

        if (m_fCircular)
        {
            m_rRadius = sqrt(rFactor);
            m_oMatrix.Reset();
        }
        else
        {
            fEmpty = !m_oMatrix.Finalize(rEmptyThresholdSquared, OUT m_oInverse);
            m_rRadius = 1;
        }
    }
    else
    {
        m_fCircular = geom.IsCircular();

        if (m_fCircular)
        {
            fEmpty = w < rEmptyThreshold;
            m_rRadius = w;
        }
        else
        {
            fEmpty = !m_oMatrix.Finalize(rEmptyThresholdSquared, m_oInverse);
            m_rRadius = 1;
        }
    }

    m_rRadSquared = m_rRadius * m_rRadius;

    m_rRefinementThreshold = ComputeRefinementThreshold(rMaxRadiusBound, rTolerance);

    //
    // ComputeRefinementThreshold assumes we will be testing if (cos(a) <
    // threshold).  But instead of testing cos(a) we will test V*W =
    // |V|*|W|*cos(a).  V and W will be radius vectors, of length m_rRadius
    // (not transformed!). So the actual test will be if (V * W) < threshold *
    // m_rRadius^2, hence:   
    //
    m_rRefinementThreshold *= m_rRadSquared;

exit:
    return !fEmpty;
}    

//+-----------------------------------------------------------------------------
//
//  Member:
//      CPen::ComputeRefinementThreshold
//
//  Synopsis:
//      Computes the threshold for deciding when the outline of a stroke needs
//      to be refined. Such a refinement may be necessary, because thick
//      strokes can magnify the (otherwise invisible) polygonalization
//      performed by Bezier flattening.
//
//  Notes:
//      This is a private method, called when the pen is set up.
//
//      The pen's nominal shape is a circle. If there is a transformation then
//      it's an ellipse, which is a projection of the circle whose radius r is
//      obtained from the nominal radius by the maximal maginification factor of
//      the transformation. The error between the arc and the chord defined by
//      the two directions is r*(1-cos(a/2)), where a is the angle between the
//      vectors. In that circle the angle between the vectors is equal to the
//      angle between the original radius vectors.  We will test if
//
//                r*(1 - cos(a/2)) <? tolerance.
//      or
//                cos(a/2) >? 1 - tolerance/r.
//      But                    ________________
//                cos(a/2) = \/(1 + cos(a)) / 2 
//      So
//                cos(a) >? 2*(1 - tolerance/r)^2 - 1
//
//      We will refine the flattening whenever cos(a) < threshold.
//
//------------------------------------------------------------------------------
GpReal
CPen::ComputeRefinementThreshold(
    GpReal rMaxRadiusBound,
        // Bound on the maximum radius of the pen
    GpReal rTolerance
        // Approximation tolerance
    )
{
    GpReal rThreshold;

    Assert(!(rMaxRadiusBound < 0));
    Assert(!(rTolerance < 0));

    if (rMaxRadiusBound < rTolerance)
    {
        //
        // The radius is less than tolerance -- we'll never need rounding.  To
        // make the test "if (cos(a) < m_rRefinementThreshold)" always fail:
        //
        rThreshold = -2;
    }
    else
    {
        rThreshold = (1 - rTolerance / rMaxRadiusBound);
        rThreshold = 2 * rThreshold * rThreshold - 1;
    }
        
    return rThreshold;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Accept a point on a Bezier segment
//
//------------------------------------------------------------------------------
HRESULT 
CPen::AcceptCurvePoint(
    __in_ecount(1) const GpPointR &point,
        // In: The point
    __in_ecount(1) const GpPointR &vecTangent,
        // In: The tangent there
    bool fLast
        // In: Is this the last point on the curve?
    )
{
    HRESULT hr = S_OK;
    GpPointR vecSeg(m_ptPrev, point);

    GpPointR vecRad;

    if (vecTangent * vecTangent >= FUZZ * FUZZ)
    {
        IFC(ComputeRadiusVector(vecTangent, vecRad));
    }
    else
    {
        //
        // (Near) 0 derivative on the curve. For lack of other options we'll
        // assume that the curve has not changed direction. Note that it is
        // *not* okay to simply skip this point, as it's likely that the curve
        // will double back on itself in the next step. In which case:
        //
        //     a) This is an extremum of the curve, so ignoring this point
        //        could result in a noticeably shorter curve.
        //     b) The point that follows this one may be identical to the one
        //        that precedes this one. In which case, we may not perform
        //        curve rounding correctly.
        //
        vecRad = m_vecRad;
    }

    //
    // If the stroke is thick enough, small corners in the skeleton
    // curve will be magnified greatly on the outside of the stroke.
    // If so, we add additional Beziers to smooth it out.
    //
    // NOTE: This is an expensive operation that not only introduces new
    // Beziers, but can also allocate new figures. Hence, it's worth checking
    // if this fixup will be in the viewable region.
    //

    if ((m_vecRad * vecRad < m_rRefinementThreshold) &&
        // Optimization checks:
        (!m_fViewableSpecified ||
         m_rcViewableInflated.DoesIntersectInclusive(
             CMilRectF(CMilPoint2F(m_ptPrev),
                          CMilPoint2F(point)))
        ))
    {
        //
        // Round the corner from the previous direction to the new segment
        //

        GpPointR vecSegRad; // The radius vector corresponding to vecSeg

        if (vecSeg * vecSeg >= FUZZ * FUZZ)
        {
            IFC(ComputeRadiusVector(vecSeg, vecSegRad));
            IFC(RoundTo(vecSegRad, m_ptPrev, m_vecPrev, vecSeg));
        }

        //
        // Draw the new segment
        //

        IFC(ProcessCurvePoint(point, vecSeg));

        //
        // Round the corner from the segment to the next tangent direction
        //

        IFC(RoundTo(vecRad, point, vecSeg, vecTangent));

        //
        // Note that RoundTo updates the current position of the outer rail,
        // but the inner rail remains untouched and is now incorrect. This is
        // fine if the next point is also on the curve, since the next
        // ProcessCurvePoint will correct this. If this is the last point on
        // the curve, however, we need one final ProcessCurvePoint.
        // 

        if (fLast)
        {
            IFC(ProcessCurvePoint(point, vecTangent));
        }
    }
    else
    {
        // Just draw the new segment, the corner is smooth enough
        SetRadiusVector(vecRad);
        IFC(ProcessCurvePoint(point, vecSeg));
    }

Cleanup:
    m_vecPrev = vecTangent;
    m_ptPrev = point;
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Get the radius vector in pen coordinates for a given direction in world
//      coordinates.
//
//      The pen shape is defined by a circle in its own coordinate space, with a
//      transformation M to world coordinates that may turn it into an ellipse. 
//      The ray at a given a world direction V intersects that ellipse at a
//      point.  This method finds the inverse image of that point on the pen's
//      circle in pen coordinates.
//
//      Let us denote M' = the inverse of M.  Then the inverse image of V is
//      VM'. A vector of length r (=the radius pen's circle) in the same
//      direction is W = (r / |VM'|) VM'.
//
//------------------------------------------------------------------------------
HRESULT 
CPen::ComputeRadiusVector(        
    __in_ecount(1) const GpPointR &vecDirection,
        // In: A not necessarily unit vector
    __out_ecount(1) GpPointR &vecRad
        // Out: Radius vector on the pen-space circle
    ) const
{
    HRESULT hr = S_OK;

    // De-transform, if necessary
    vecRad = vecDirection;
    if (!m_fCircular)
        m_oInverse.Transform(vecRad);

    // Set to the right length

    GpReal rLength = vecRad.Norm();

    //
    // Callers are expected to check that vecDirection isn't small, so
    // rLength shouldn't be 0 at this point, but it's conceivable one could
    // still sneak through.
    //
    // Note: As long as the input isn't precisely 0, the normalization
    // performed here should be numerically stable. That said, callers should
    // check to make sure that the vecDirection isn't small, as that's often an
    // indication of numerical instability elsewhere in the system. For
    // instance, the geometry or render transforms may be near-degenerate or
    // the vector may come from a 0-speed point on a Bezier.
    //
    if (rLength > 0.0 && _finite(rLength))
    {
        vecRad *= m_rRadius / rLength;
    }
    else
    {
        //
        // Future Consideration:  This really shouldn't happen
        // (callers should catch this case). Consider replacing with
        // an assert in MILv2.
        //
        IFC(WGXERR_BADNUMBER);
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Set the radius vector member to a given value, and update the current
//      offset information for that value.  See ComputeRadiusVector for the
//      definition of that vector.
//
//------------------------------------------------------------------------------
void
CPen::SetRadiusVector(
    __in_ecount(1) const GpPointR &vecRad
        // In: A Given radius vector
    )
{
    // It is assumed that the caller is passing a legitimate radius vector
    #if DBG
    Assert(vecRad.DbgIsOfLength(m_rRadius, .01));
    #endif

    m_vecRad = vecRad;
    GetOffsetVector(vecRad, m_vecOffset);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Update the offset information for a given direction vector on the path
//
//------------------------------------------------------------------------------
HRESULT
CPen::UpdateOffset(
    __in_ecount(1) const GpPointR &vecDirection
        // In: A nonzero direction vector
    )
{
    HRESULT hr;

    Assert(vecDirection * vecDirection != 0.0);

    IFC(ComputeRadiusVector(vecDirection, m_vecRad));

    GetOffsetVector(m_vecRad, m_vecOffset);

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Get the offset vector that corresponds to a given radius vector.
//
//------------------------------------------------------------------------------
void
CPen::GetOffsetVector(
    __in_ecount(1) const GpPointR &vecRad,
        // In: A radius vector
    __out_ecount(1) GpPointR &vecOffset
        // Out: The corresponding offset vector
    ) const
{
    // Update the cached offset vector
    vecOffset = vecRad;
    vecOffset.TurnRight();
    if (!m_fCircular)
        m_oMatrix.Transform(vecOffset);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Get the point on the world space (elliptical) pen shape that corresponds
//      to a given radius vector in (circular) pen coordinates.
//
//------------------------------------------------------------------------------
GpPointR 
CPen::GetPenVector(
    __in_ecount(1) const GpPointR &vecRad
        // In: A radius vector
    ) const
{
    #if DBG
    Assert(vecRad.DbgIsOfLength(m_rRadius, .01)); // Should be a radius vector
    #endif

    GpPointR vec(vecRad);

    if (!m_fCircular)
        m_oMatrix.Transform(vec);
    return vec;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Compute the square of the pen width perpendicular to a given direction.
//
//      This method is tailored for markers.  They need to know the width of the
//      path at a given point for computing the size of the marker attached
//      there. The width of a circular pen is the same everywhere, but for an
//      elliptical pen it depends on the direction vector at that point.
//      Computing the actual width involves a square root and a division, and we
//      want to avoid that.  Our caller is actually happy with the numerator and
//      denominator of the width squared, so that's what we return.
//
//------------------------------------------------------------------------------
void 
CPen::GetSqWidth(
    __in_ecount(1) const GpPointR &V,
        // In: The given direction vector
    __out_ecount(1) GpReal &rNumerator,
        // Out: The result's numerator
    __out_ecount(1) GpReal &rDenominator
        // Out: The result's denominator
    ) const
{
    if (m_fCircular)
    {
        // The width of a circular pen = twice the radius in any direction
        rNumerator = 4 * m_rRadSquared;
        rDenominator = 1;
    }
    else
    {
        //
        // In the pen's coordinate space, the boundary of the pen is the circle
        // r(cos(t), sin(t)), where r is the circle's radius. The boundary of
        // the pen's ellipse is the locus of r(cos(t), sin(t))M for all t,
        // where M is the pen transform matrix. The width of that ellipse in
        // the direction of the vector V is the maximum of the function
        //  
        //     f(t) = 2r(cos(t), sin(t))M * (V/|V|),
        //      
        // where * is the dot product. But the dot product A*B can be written
        // as the matrix multiplication AB`, where ` stands for transpose, so
        // f(t) can be be rewritten as 
        //         
        //     f(t) = 2r(cos(t), sin(t))(MV`)/|V|,
        // 
        // The maximum of that is attained  where f'(s)=0, that is
        // 
        //     (-sin(s), cos(s))(MV`) = 0,
        //     
        // which can be rewritten as (-sin(s), cos(s)) * (VM`)
        //     
        // That will happen if (cos(s), sin(s)) = VM`/|VM`|, and then
        // 
        //     f(s) = 2r(cos(s), sin(s))MV`/|V| = 2rVM`MV`/(|VM`||V|) = 
        //            2r(VM`)(VM)`/(|VM`||V|) = 2r|VM`|/|V|.
        // 
        // The square of this is 4r^2(W*W)/(V*V), where W=VM'.
        //     
        // Here we return the squares of the numerator and denominator
        //

        GpPointR W = V;
        W.TurnRight(); // Direction perpendicular to V
        m_oMatrix.TransformColumn(W);

        rNumerator = 4 * m_rRadSquared * (W * W);
        rDenominator = V * V;
    }
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Compute the numbers associated with the turning angle
//
//      This private method analyzes the corner vectors.  If they are parallel
//      it returns false. Otherwise it computes their determinant and dot
//      product, and determines the outer side of the turn.
//
//------------------------------------------------------------------------------
bool
// Return false if this is not a turn at all
CPen::GetTurningInfo(
    __in_ecount(1) const GpPointR &vecIn,
        // In: Vector in the direction comin in
    __in_ecount(1) const GpPointR &vecOut,
        // In: Vector in the direction going out
    __out_ecount(1) GpReal &rDet,
        // Out: The determinant of the vectors
    __out_ecount(1) GpReal &rDot,
        // Out: The dot product of the vectors
    __out_ecount(1) RAIL_SIDE &side,
        // Out: The outer side of the turn
    __out_ecount(1) bool &f180Degrees
        // Out: =true if this is a 180 degrees turn
    ) const
{
    side = RAIL_RIGHT;
    f180Degrees = false;
    
    rDet = Determinant(vecIn, vecOut);
    rDot = vecIn * vecOut;
    
    if (fabs(rDet) <= fabs(rDot) * SQ_LENGTH_FUZZ)
    {
        if (vecIn * vecOut > 0)
            return false;
        else
            f180Degrees = true;
    }
    else
    {   
        if (rDet > 0)
            side = RAIL_LEFT;    // In a right handed coordinate system
        else
            side = RAIL_RIGHT;   // Turning left
    }
    return true;
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CSimplePen
//  
//  Synopsis:
//      Implements an (undashed, simple) pen.
//
//  Notes:
//      The alternative is a (speced, but not implemented) complex pen, which
//      would allow multiple "prongs" on the pen (like a rake).
//
//      Some of the methods should be pushed up to the base class CPen.
//

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Set the current left & right points to given values.
//
//------------------------------------------------------------------------------
HRESULT 
CSimplePen::SetCurrentPoints(
    __in_ecount(1) const GpPointR &ptLeft,
        // In: Left point
    __in_ecount(1) const GpPointR &ptRight
        // In: Right point
    )
{
    m_ptCurrent[0] = ptLeft;
    m_ptCurrent[1] = ptRight;

    RRETURN(m_pSink->SetCurrentPoints(m_ptCurrent));
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Extends one of the sides to a given point.
//
//------------------------------------------------------------------------------
HRESULT 
CSimplePen::MiterTo(
    RAIL_SIDE side,
        // Which side to set the point
    __in_ecount(1) const GpPointR &ptMiter,
        // Miter corner
    __in_ecount(1) const GpPointR &ptNextStart,
        // The starting point of the next segment's offset
    bool fExtended)
        // Extend all the way to ptNextStart if true
{  

    //
    // To save the cost of an extra segment per corner, a normal corner is
    // extended to the miter point only.  If this is the last corner in a
    // closed figure and the cap is flat, this may leave a wedge-shaped gap, as
    // illustrated below:
    //
    //                                      gap
    //                                      . .-----
    //                                      |\|
    //                                      | |\| Starting segment
    //                                      |  -|-- 
    //                                      |   | 
    //                        Ending segment
    //
    // Called with fExtended in that case will bridge that gap by going all the
    // way to the start of the next segment's offset.
    //
    
    HRESULT hr;
    if (fExtended)
    {
        GpPointR P[2] = {ptMiter, ptNextStart};
        IFC(m_pSink->PolylineWedge(side, 2, P));
        m_ptCurrent[side] = ptNextStart;
    }
    else
    {
        IFC(m_pSink->PolylineWedge(side, 1, &ptMiter));
        m_ptCurrent[side] = ptMiter;
    }

Cleanup:
    RRETURN(hr);

}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Start the widening of a new figure
//
//------------------------------------------------------------------------------
HRESULT 
CSimplePen::StartFigure(
    __in_ecount(1) const GpPointR &pt,
        // In: Figure's first point
    __in_ecount(1) const GpPointR &vecSeg,
        // In: First segment's (non-zero) direction vector
    bool,
        // Ignored here
    MilPenCap::Enum eCapType
        // In: The start cap type
    )
{
    HRESULT hr;

    Assert(RAIL_LEFT == 0  &&  RAIL_RIGHT == 1);
    Assert(vecSeg * vecSeg != 0.0);

    WIDEN_TRACE(L"CSimplePen::StartFigure\n");

    // Set the offset vector and current offset point
    IFC(UpdateOffset(vecSeg));

    m_ptPrev = pt;
    m_vecPrev = vecSeg;
    m_ptCurrent[RAIL_LEFT] = pt - m_vecOffset;
    m_ptCurrent[RAIL_RIGHT] = pt + m_vecOffset;

    IFC(m_pSink->StartWith(m_ptCurrent));

    IFC(DoBaseCap(RAIL_START, pt, -vecSeg, eCapType));
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Accept a point on a line segment
//
//------------------------------------------------------------------------------
HRESULT 
CSimplePen::AcceptLinePoint(
    __in_ecount(1) const GpPointR &point
        // In: Point to draw to
    )
{
    m_ptCurrent[0] = point - m_vecOffset;
    m_ptCurrent[1] = point + m_vecOffset;
    m_ptPrev = point;
    return m_pSink->QuadTo(m_ptCurrent);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Process a point on a curve
//
//------------------------------------------------------------------------------
HRESULT 
CSimplePen::ProcessCurvePoint(
    __in_ecount(1) const GpPointR &point,
        // In: Point to draw to
    __in_ecount(1) const GpPointR &vecSeg
        // In: Direction of segment we're coming along
    )
{
    m_ptCurrent[0] = point - m_vecOffset;
    m_ptCurrent[1] = point + m_vecOffset;
    return m_pSink->QuadTo(m_ptCurrent, vecSeg, point, m_ptPrev);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Do a start or end base cap.  A base cap connects the two rails of the
//      widened path to close the widening outline.
//
//------------------------------------------------------------------------------
HRESULT 
CSimplePen::DoBaseCap(
    RAIL_TERMINAL whichEnd,
        // In: RAIL_START or RAIL_END
    __in_ecount(1) const GpPointR &ptCenter,
        // In: Cap's center
    __in_ecount(1) const GpPointR &vec,
        // In: Segment's direction vector at this point
    MilPenCap::Enum type
        // In: The type of cap
    )
{
    HRESULT hr = S_OK;

    switch (type)
    {
    case MilPenCap::Square:
        IFC(DoSquareCap(whichEnd, ptCenter));
        break;

    case MilPenCap::Flat:
        IFC(m_pSink->CapFlat(m_ptCurrent, TERMINAL2SIDE(whichEnd)));
        break;

    case MilPenCap::Triangle:
        {
            GpPointR P = GetPenVector(m_vecRad);
            if (RAIL_START == whichEnd)
                P = ptCenter - P;
            else
                P += ptCenter;    
            IFC(m_pSink->CapTriangle(m_ptCurrent[OPPOSITE_SIDE(TERMINAL2SIDE(whichEnd))], 
                                     P, 
                                     m_ptCurrent[TERMINAL2SIDE(whichEnd)]));
            break;
        }

    case MilPenCap::Round:
        IFC(DoRoundCap(whichEnd, ptCenter));
        break;
    }
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Compute a square line cap
//
//------------------------------------------------------------------------------
HRESULT 
CSimplePen::DoSquareCap(
    RAIL_TERMINAL whichEnd,
        // RAIL_START or RAIL_END
    __in_ecount(1) const GpPointR &ptCenter
        // Cap's center
    )
{
    HRESULT hr;

    GpPointR V = GetPenVector(m_vecRad);

    if (RAIL_START == whichEnd)
    {
        // Record the current start
        GpPointR ptStart = m_ptPrev;
        GpPointR ptStartOffsets[2] = {m_ptCurrent[RAIL_LEFT], m_ptCurrent[RAIL_RIGHT]};

        // Move the start back by V
        IFC(SetCurrentPoints(m_ptCurrent[RAIL_LEFT] - V, m_ptCurrent[RAIL_RIGHT] - V));
        m_ptPrev -= V;

        // Start from there and fill a quad to the previous start
        IFC(m_pSink->CapFlat(m_ptCurrent, TERMINAL2SIDE(RAIL_START)));
        IFC(m_pSink->QuadTo(ptStartOffsets));

        // Restore current start
        m_ptPrev = ptStart;
        m_ptCurrent[RAIL_LEFT] = ptStartOffsets[RAIL_LEFT];
        m_ptCurrent[RAIL_RIGHT] = ptStartOffsets[RAIL_RIGHT];
    }
    else
    {
        // Draw a line segment in the direction of V and cap
        IFC(AcceptLinePoint(m_ptPrev + V));
        IFC(m_pSink->CapFlat(m_ptCurrent, TERMINAL2SIDE(RAIL_END)));
    }
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Compute a round line cap
//
//------------------------------------------------------------------------------
HRESULT 
CSimplePen::DoRoundCap(
    RAIL_TERMINAL whichEnd,
        // In: RAIL_START or RAIL_END
    __in_ecount(1) const GpPointR &ptCenter
        // In: Cap's center
    )
{
    // Mainly for code readability:
    const GpPointR &ptEnd = m_ptCurrent[TERMINAL2SIDE(whichEnd)];
    const GpPointR &ptStart = m_ptCurrent[OPPOSITE_SIDE(TERMINAL2SIDE(whichEnd))];

    // Construct 2 Bezier arcs
    GpPointR vecAcross(ptCenter, ptEnd);    // = ptEnd - ptCenter
    vecAcross *= ARC_AS_BEZIER;
    GpPointR vecAlong = GetPenVector(m_vecRad);
    if (RAIL_START == whichEnd)
        vecAlong = -vecAlong;
    GpPointR ptMid = ptCenter + vecAlong;
    vecAlong *= ARC_AS_BEZIER;

    return m_pSink->BezierCap(ptStart,
                              ptStart + vecAlong,
                              ptMid - vecAcross, 
                              ptMid, 
                              ptMid + vecAcross,
                              ptEnd + vecAlong,
                              ptEnd);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Compute the contour of a mitered, rounded or beveled corner on the
//      widened path.
//
//------------------------------------------------------------------------------
HRESULT
CSimplePen::DoCorner(
    __in_ecount(1) const GpPointR &ptCenter,
        // Corner center point
    __in_ecount(1) const GpPointR &vecIn,
        // Vector in the direction coming in
    __in_ecount(1) const GpPointR &vecOut,
        // Vector in the direction going out
    MilLineJoin::Enum eLineJoin,
        // Corner type
    bool fSkipped,
        // =true if this corner straddles a degenerate segment 
    bool fRound,
        // Enforce rounded corner if true
    bool fClosing
        // This is the last corner in a closed figure if true
    )
{
    GpPointR vecRad;
    GpPointR vecOffset, ptNext[2];
    HRESULT hr = S_OK;

    WIDEN_TRACE(L"CSimplePen::DoCorner\n");


    GpReal rSavedMiterLimit = 0, rSavedNominalMiterLimit = 0, rSavedMiterLimitSquared = 0;
    
    if (fRound)
    {
        // Enforce a rounded corner
        eLineJoin = MilLineJoin::Round;
    }
    else if (fSkipped  &&  eLineJoin != MilLineJoin::Round)
    {
        //
        // This corner straddles a degenerate edge so we want to miter it with
        // miter limit 1; that will look as if a very short edge is being widened
        //
        
        // Save the normal values
        rSavedMiterLimit = m_rMiterLimit;
        rSavedNominalMiterLimit = m_rNominalMiterLimit;
        rSavedMiterLimitSquared = m_rMiterLimitSquared;

        // Set to miter with miter limit 1
        m_rNominalMiterLimit = 1;
        m_rMiterLimit = m_rRadius;
        m_rMiterLimitSquared = m_rRadSquared;
        eLineJoin = MilLineJoin::Miter;
    }

    // 
    // DoCorner is called after a segment has been processed. However, a
    // segment should only be processed if it has non-zero length. 
    //
    Assert(vecOut * vecOut != 0.0);
    IFC(ComputeRadiusVector(vecOut, vecRad));

    // Get the new radius vector and offset points on the outgoing segment
    GetOffsetVector(vecRad, vecOffset);
    ptNext[0] = ptCenter - vecOffset;
    ptNext[1] = ptCenter + vecOffset;
    
    RAIL_SIDE side;  // The outer side of the turn
    bool f180Degrees;
    GpReal rDet, rDot;

    if (!GetTurningInfo(vecIn, vecOut, rDet, rDot, side, f180Degrees))
    {
        // It's a flat join, stay with the current points
        goto Cleanup;
    }
    
    // Now do the outside corner
    switch (eLineJoin)
    {
    case MilLineJoin::MiterClipped:
        if (f180Degrees)
        {
            IFC(m_pSink->SwitchSides());
        }
        else
        {
            IFC(m_pSink->DoInnerCorner(OPPOSITE_SIDE(side),ptCenter, ptNext));

            GpPointR ptMiter;
            if (GetMiterPoint(vecRad, 
                              rDet, 
                              m_ptCurrent[side], 
                              vecIn, 
                              ptNext[side], 
                              vecOut, 
                              rDot, 
                              ptMiter))
            {
                // Miter the corner
                IFC(MiterTo(side, ptMiter, ptNext[side], fClosing));
            }
            else
            {
                // Miter failed or exceeds the limit, so bevel the corner 
                IFC(BevelCorner(side, ptNext[side]));
            }
        }
        break;

    case MilLineJoin::Bevel:
        if (f180Degrees)
        {
            IFC(m_pSink->SwitchSides());
        }
        else
        {
            IFC(m_pSink->DoInnerCorner(OPPOSITE_SIDE(side),ptCenter, ptNext));
            IFC(BevelCorner(side, ptNext[side]));
        }
        break;

    case (MilLineJoin::Miter):
        if (f180Degrees)
        {
            IFC(Do180DegreesMiter());
        }
        else
        {
            IFC(m_pSink->DoInnerCorner(OPPOSITE_SIDE(side),ptCenter, ptNext));

            GpPointR ptMiter;
            if (GetMiterPoint(vecRad, 
                              rDet, 
                              m_ptCurrent[side],
                              vecIn,
                              ptNext[side], 
                              vecOut, 
                              rDot, 
                              ptMiter))
            {
                // Miter the corner
                IFC(MiterTo(side, ptMiter, ptNext[side], fClosing));
            }
            else
            {
                // Miter length exceeds the limit, so clip it
                IFC(DoLimitedMiter(m_ptCurrent[side], 
                                    ptNext[side], 
                                    rDot, 
                                    vecRad, 
                                    side));
            }
        }
        break;

    case (MilLineJoin::Round):
        IFC(m_pSink->DoInnerCorner(OPPOSITE_SIDE(side),ptCenter, ptNext));
        IFC(RoundCorner(ptCenter, m_ptCurrent[side], 
                             ptNext[side], vecRad, side));
        break;
    }
    
    // Update for the next segment
    m_vecRad = vecRad;
    m_vecOffset = vecOffset;
    m_ptPrev = ptCenter;
    m_vecPrev = vecOut;

Cleanup:
    if (fSkipped  &&  eLineJoin != MilLineJoin::Round)
    {
        // Restore the miter settings
        m_rMiterLimit = rSavedMiterLimit;
        m_rNominalMiterLimit = rSavedNominalMiterLimit;
        m_rMiterLimitSquared = rSavedMiterLimitSquared;
    }
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      This is a private method called by DoCorner.  It computes the clipping
//      line that cuts the corner when the miter length exceeds the miter limit.
//
//      In pen coordinates, we are looking for a line that clips the corner,
//      whose distance from the spine corner will be equal to
//      m_rMiterLimit*m_rRadius. the vector along the outer offset from the
//      offset point to the clip point is ratio * radius vector.  So we compute
//      this ratio, and then apply it to the the radius vector transformed to
//      world coordinates.  The result will take us from the offset point to the
//      clip point.
//
//  Notations:
//      a = the angle between the legs of the corner L = miter limit r = pen
//      radius (=1/2 line width) s = the distance from the offset point to the
//      clip point on the offset line dot = -dot product of the radius vectors
//
//              offset point
//           --*----------------------  offset line
//          | *
//          |*
//       -  * clip point
//       s  |                 spine
//       -  * offset  ................          vecRadNext
//          | point   . a                       ------>
//          |         .
//          |         .                         
//          |         .                        /|\
//          |  - r -  .        -------          |
//          |         .       |                 | m_vecRad
//          |         .       |                 |
//
//  and we want to compute s / r.
//
//  By trigonometry, s = (L*r - r sin(a/2)) / cos(a/2), so
//
//        s     L - sin(a/2)
//       --- = --------------- 
//        r      cos(a/2)
//
//  The trig formulas for half angle are:
//                   ________________                      ________________                           
//      cos(a/2) = \/(1 - cos(a)) / 2   and   sin(a/2) = \/(1 + cos(a)) / 2  
//
//  and cos(a) = dot/r^2, so
//                    ___________________            _______________
//        s     L - \/(1 - dot / r^2) / 2    L*r - \/(r^2 - dot) / 2
//       --- = ----===================--- = ----===============-----
//        r      \/(1 + dot / r^2) / 2        \/(r^2 + dot) / 2
//
//  The deniminator is 0 when a = 180, hence the corner is flat, so we treat it
//  as no-corner and do nothing.  (Should never happen, but remember Murphey...)
//
//------------------------------------------------------------------------------
HRESULT
CSimplePen::DoLimitedMiter(
    __in_ecount(1) const GpPointR &ptIn,
        // In: Outer offset of incoming segment
    __in_ecount(1) const GpPointR &ptNext,
        // In: Outer offset of outgoing segment
    GpReal rDot,
        // In: -(m_vecRad * vecRadNext)
    __in_ecount(1) const GpPointR &vecRadNext,
        // In: Radius vector of outgoing segment
    RAIL_SIDE side
        // In: Turn's outer side, RAIL_LEFT or RAIL_RIGHT
    )
{
    HRESULT hr = S_OK;

    GpReal rDenom = (m_rRadSquared + rDot) / 2;

    if (rDenom > 0) // Otherwise it's not really a corner
    {
        rDenom = sqrt(rDenom);
        GpReal rRatio = (m_rRadSquared - rDot) / 2;   // Numerator
        if (rRatio < 0)
            rRatio = 0;
        else
            rRatio = sqrt(rRatio);
        rRatio = m_rMiterLimit  - rRatio;
        if (rRatio < 0)  // Shouldn't happen but...
            rRatio = 0; 

        if (rDenom > rRatio * FUZZ) // Otherwise it's not really a corner
        {
            rRatio /= rDenom;
            GpPointR V = GetPenVector(m_vecRad);
            GpPointR W = GetPenVector(vecRadNext);

            // Generate the bevel
            GpPointR P[3] = {m_ptCurrent[side] + V * rRatio, 
                             ptNext - W * rRatio,
                             ptNext};

            m_ptCurrent[side] = ptNext;
            IFC(m_pSink->PolylineWedge(side, 3, P));
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Miter the corner when the turning angle is 180 degrees. This is a
//      private method called by DoCorner. Since there is not really a corner,
//      we don't really miter.  Instead, we move the points outward, connecting
//      left to right and right to left
//
//------------------------------------------------------------------------------
HRESULT
CSimplePen::Do180DegreesMiter()
{
    HRESULT hr = S_OK;
    // First move the current points outwards to the miter limit
    GpPointR vec = GetPenVector(m_vecRad);
    vec *= m_rNominalMiterLimit;
    IFC(SetCurrentPoints(m_ptCurrent[0] + vec, m_ptCurrent[1] + vec));
    IFC(m_pSink->SwitchSides());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Bevel the corner
//
//------------------------------------------------------------------------------
HRESULT
CSimplePen::BevelCorner(
    RAIL_SIDE side,
        // In: The side of the outer corner
    __in_ecount(1) const GpPointR &ptNext
        // In: The bevel's endpoint
    )
{
    m_ptCurrent[side] = ptNext;
    RRETURN(m_pSink->PolylineWedge(side, 1, &ptNext));
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Compute the outline of the arc that rounds the outer corner. This is a
//      private method called by DoCorner.
//
//      The arc is approximated by 1 or 2 Bezier curves, depending on the turn
//      angle. The computation is done in the pen coordinates (where the pen
//      shape is circular) and then transformed to world coordinates (where the
//      pen is elliptical).  The curve's endpoints obviously coincide with the
//      arc's endpoints, which are at the tips of the start and end
//      radius-vectors.  The control points are on the tangent lines there,
//      whose directions are the radius vectors turned 90 degrees left or right,
//      depending on the turning direction. It remains to compute where on these
//      tangent the control points should be placed.
//
//------------------------------------------------------------------------------
HRESULT
CSimplePen::RoundCorner(
    __in_ecount(1) const GpPointR &ptCenter,
        // In: Corner point on the spine
    __in_ecount(1) const GpPointR &ptIn,
        // In: Outer offset point of incoming segment
    __in_ecount(1) const GpPointR &ptNext,
        // In: Outer offset point of outgoing segment
    __in_ecount(1) const GpPointR &vecRad,
        // In: New value of m_vecRad
    RAIL_SIDE side
        // In: Side to be rounded, RAIL_LEFT or RAIL_RIGHT
    )
{ 
    #if DBG
    Assert(vecRad.DbgIsOfLength(m_rRadius, .01)); // Should be a radius vector
    #endif

    HRESULT hr = S_OK;

    GpReal r = vecRad * m_vecRad;   // = rad^2 * cos(angle between radius vecs)
    if (r > m_rRefinementThreshold)
    {
        // A very flat turn, the arc can be approximated by the bevel
        IFC(m_pSink->PolylineWedge(side, 1, &ptNext));
    }
    else if (r >= 0)
    {
        // The arc can be approximated by a single Bezier curve
        r = GetBezierDistance(r, m_rRadius);
        GpPointR ptBez1 = ptIn + GetPenVector(m_vecRad) * r;
        GpPointR ptBez2 = ptNext - GetPenVector(vecRad) * r;
        IFC(m_pSink->CurveWedge(side, ptBez1, ptBez2, ptNext)); 
    }
    else
    {
        // Need to approximate the arc by 2 Bezier curves ---

        //
        // Get the radius vector for the arc's midpoint
        //
        // We use a little complex arithmetic here. Given two equal length
        // vectors, a and b (each represented as a complex number x + iy), the
        // midpoint, c, is given by sqrt(a * b). Note that -c is also a
        // midpoint of a and b, so we need to do a check at the end to see
        // which one we need.
        //
        // In this case, a is vecRad and b is m_vecRad.
        //

        // Real component of c^2
        GpReal c2Real = vecRad.X * m_vecRad.X - vecRad.Y * m_vecRad.Y;

        // Imaginary component of c^2
        GpReal c2Imag = vecRad.X * m_vecRad.Y + vecRad.Y * m_vecRad.X;

        //
        // The square root of a complex number x + iy is given by the formula:
        //
        //      sqrt( (L + x)/2 ) + i sgn(y) sqrt( (L - x)/2 )
        //
        // Where L is the length of the vector (x,y) and sgn() is the sign
        // operator:
        //
        //               / +1   (t > 0)
        //      sgn(t) = |  0   (t == 0)
        //               \ -1   (t < 0)
        //
        // We can ignore the behavior of sgn(t) at 0, though, because when y
        // == 0, x == L and hence the value of the sqrt() will be 0 anyway. We
        // can also assume that |c2Real| is less than L, since no component of
        // a vector is greater than its length. Due to numerical error,
        // though, this might not actually hold. To ensure that we don't take
        // the square root of a negative number, we take absolute values
        // first.
        //

        GpReal L = m_rRadius * m_rRadius; // |a*b| = |a|*|b|
        GpReal cReal = sqrt(abs(0.5 *(L + c2Real)));
        GpReal cImag = (c2Imag > 0.0 ? 1.0 : -1.0) * sqrt(abs(0.5 *(L - c2Real)));
        
        GpPointR vecMid(cReal, cImag);

        //
        // At this point, vecMid may be pointing in the opposite direction than
        // desired (remember that c and -c from the above discussion are both
        // square roots of a*b).
        //
        // Rotating vecRad by 90 degrees in the direction in which the curve
        // should be added will give us roughly the direction in which vecMid
        // should be pointed ( plus or minus 45 degrees ). We can thus use the
        // dot product to determine whether we need to negate vecMid.
        //

        GpPointR direction = vecRad;
        direction.TurnRight();
        if (RAIL_LEFT == side)
        {
            direction = -direction;
        }

        if (vecMid * direction < 0.0f)
        {
            vecMid = -vecMid;
        }

        //
        // vecMid *should* now be pointing in the right direction but
        // unfortunately large stretch transforms can cause the angle between
        // vecIn and vecOut to be very close to 180 degrees. When inverting
        // this transform, the angle between m_vecRad and vecRad may be on the
        // *opposite* side of 180 degrees (but need not be close to 180). In
        // this case, the angle between vecMid and vecRad may be well over 90
        // degrees, which is inconsistent. Since vecIn and vecOut are closer to
        // what will actually be drawn, we trust their values and just assume
        // that vecMid really is pointing in the right direction.
        //
        GpReal radDotMid = fabs(vecRad * vecMid);
        
        // Get the relative distance to the control points
        r = GetBezierDistance(radDotMid, m_rRadius);

        // Get the arc's midpoint as the tip of the offset in this direction
        GpPointR ptMid = vecMid;
        ptMid.TurnRight();
        if (RAIL_LEFT == side)
            ptMid = -ptMid;
        if (!m_fCircular)
            m_oMatrix.Transform(ptMid);
        ptMid += ptCenter;

        // First arc, from ptIn to ptMid
        GpPointR ptBez = ptIn + GetPenVector(m_vecRad) * r;
        GpPointR vecBezAtMid = GetPenVector(vecMid) * r;
        IFC(m_pSink->CurveWedge(side, ptBez, ptMid - vecBezAtMid, ptMid));

        // Second arc, from ptMid ptNext
        ptBez = ptNext - GetPenVector(vecRad) * r;
        IFC(m_pSink->CurveWedge(side, ptMid + vecBezAtMid, ptBez, ptNext)); 
    }

Cleanup:
    m_ptCurrent[side] = ptNext;
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Get the outer miter point, if legitimate and within miter limit. In any
//      case, compute the dot product
//
//      Failure to compute a miter point (and returning false) is not a big
//      deal. It should only happen if vecIn and vecOut are almost collinear,
//      and then the caller will gloss over the corner (if rDot > 0) or handle
//      it as a 180 degree turn (if rDet <= 0).
//
//  Returns:
//      False if miter-limit is exceeded.
//
//------------------------------------------------------------------------------
bool 
CSimplePen::GetMiterPoint(      
    __in_ecount(1) const GpPointR &vecRad,
        // Radius vector for the outgoing segment
    GpReal rDet,
        // The determinant of vecIn and vecOut
    __in_ecount(1) const GpPointR &ptIn,
        // Offset point of the incoming segment
    __in_ecount(1) const GpPointR &vecIn,
        // Vector in the direction coming in
    __in_ecount(1) const GpPointR &ptNext,
        // Offset point of the outgoing segment
    __in_ecount(1) const GpPointR &vecOut,
        // Vector in the direction going out
    __out_ecount(1) GpReal &rDot,
        // The dot product of the 2 radius vectors
    __out_ecount(1) GpPointR &ptMiter
        // The outer miter point, if within limit
    )
{
    bool fDone = false;

    rDot = -vecRad * m_vecRad;

    //
    // The miter point is the intersection of the extensions of the two offset
    // segments:
    //    {ptIn rIn * vecIn} and {ptNext + rOut * vecOut}.
    //
    // To computed the intersection, solve the equations:
    //
    //               ptIn + rIn * vecIn = ptNext + rOut * vecOut
    // or:
    //               rIn * vecIn - rOut * vecOut = ptNext - ptIn
    //
    // The unknowns are rIn and rOut.  Since we have already chose to be on the
    // outer offset, we expect the intersection point to be in the correct
    // extensions of the offset segments - forward from ptIn on the incoming
    // segment and backward from ptOut on the outgoing one.  This translates to
    // rIn > 0 and rOut < 0.  Numerical error may produces a bad point, which
    // may show up as a spike, so we guard against it. 
    //
    // The vector equation represents 2 scalar equations in rIn and rOut.
    // By Cramer's rule the solution is: 
    //
    //     rIn = det(ptNext - ptIn, -vecOut) / det(vecIn, -vecOut)
    //     rOut = det(vecIn, ptNext - ptIn) / det(vecIn, -vecOut)
    //
    // After some basic algebra, and using pt = ptNext - ptIn:
    //
    //     rIn = det(pt, vecOut) / det(vecIn, vecOut)
    //     rOut = det(pt, vecIn) / det(vecIn, vecOut)
    //

    GpPointR pt(ptIn, ptNext); 
    GpReal rIn;
    GpReal rInNumerator = Determinant(pt, vecOut);
    GpReal rOutNumerator = Determinant(pt, vecIn);

    //
    // We don't need rOut, we only need to check its sign, so instead of the
    // fraction's sign we examine the signs of its numerator and denominator.
    // For rIn we will eventually need to divide, but we can save the cost
    // division if we determine that it will fail by examining the numerator
    // and denominator.
    //
    if (rDet < 0)
    {
        fDone = rInNumerator < 0    &&         // so that rIn > 0  
                rOutNumerator > 0   &&         // so that rOut < 0
                rDet < rInNumerator * FUZZ;    // so that
                                               // |rDet| > |rNumerator* FUZZ|
    }                                          // and it is safe to divide
    else    // rDet >= 0
    {
        fDone = rInNumerator > 0   &&          // so that rIn > 0 
                rOutNumerator < 0  &&          // so that rOut < 0
                rDet > rInNumerator * FUZZ;    // so that
                                               // |rDet| > |rNumerator* FUZZ|
                                               // and it is safe to divide
    }

    if (!fDone)
    {
        // The incoming and outgoing edges are almost collinear
        if (rDot < 0)
        {
            // This is a smooth join, let's just gloss over the corner.
            ptMiter = ptNext;
            fDone = true;
        }
        // else this is close to 180 degree turn, we cannot miter it.
        // Either way --
        goto exit;
    }

    rIn = rInNumerator / rDet;
    ptMiter = ptIn + vecIn * rIn;

    //
    // Miter point computed successfully
    //

    //
    // Check if this corner can be mitered with miter distance <= miter limit.
    // 
    // The test is done in pen coordinate space.  There the miter distance,
    // which is the distance from the center to the miter corner, is R /
    // sin(a/2), where R is the pen radius, and a is the angle at the corner. 
    // 
    // The test is R/sin(a/2) <= L, where L is the limit. 
    // 
    // But sin(a/2) = sqrt((1 - cos(a)) / 2), so the test is
    //
    //      R <= L*sqrt((1 - cos(a))/2), or 2R^2 <= (1 - cos(a))L^2. 
    // 
    // Eliminating cos(a), we get cos(a) <= 1 - 2R^2 / L^2.
    // 
    // Multiply both sides by R^2 and substitute R^2*cos(a) = -U*V, where U and
    // V are the radius vectors of the 2 segments, we have:
    //
    //      U*V <= R^2(1 - 2 R^2 / L^2), or (U*V)L^2 <= R^2(L^2 - 2 R^2).
    // 

    #if DBG
    Assert(vecRad.DbgIsOfLength(m_rRadius, .01)); // Should be a radius vector
    #endif

    fDone = (rDot * m_rMiterLimitSquared <= 
           m_rRadSquared * (m_rMiterLimitSquared - 2 * m_rRadSquared));
exit:
    return fDone;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSimplePen::EndStrokeOpen
//
//  Synopsis:
//      End an a stroke as a open
//
//------------------------------------------------------------------------------
HRESULT 
CSimplePen::EndStrokeOpen(
    bool fStarted,
        // = true if the widening has started
    __in_ecount(1) const GpPointR &ptEnd,
        // Figure's endpoint
    __in_ecount(1) const GpPointR &vecEnd,
        // Direction vector there
    MilPenCap::Enum eEndCap,
        // The type of the end cap
    MilPenCap::Enum eStartCap)
        // The type of start cap (optional)
{
    HRESULT hr;
    if (!fStarted)
    {
        //
        // We should be stroking but failed to start any segment, so we'll just
        // widen as a single point, with a horizontal (in shape space)
        // direction vector.                    
        //
        GpPointR vecIn(1, 0);
        m_oWToDMatrix.Transform(vecIn);

        IFC(StartFigure(ptEnd, vecIn, false, eStartCap));
    }

    IFC(DoBaseCap(RAIL_END, ptEnd, vecEnd, eEndCap));
    IFC(m_pSink->AddFigure());
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSimplePen::EndStrokeClosed
//
//  Synopsis:
//      End an a stroke as a closed
//
//------------------------------------------------------------------------------
HRESULT 
CSimplePen::EndStrokeClosed(
    __in_ecount(1) const GpPointR &ptEnd,
        // Figure's endpoint
    __in_ecount(1) const GpPointR &vecEnd)
        // Direction vector there
{
    HRESULT hr;

    IFC(DoBaseCap(RAIL_END, ptEnd, vecEnd, MilPenCap::Flat));
    IFC(m_pSink->AddFigure());
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSimplePen::Aborted
//
//  Synopsis:
//      Say if the widening has been aborted
//
//  Notes:
//      This is only used by CHitTestWideningSink for early out when a hit has
//      been detected.  It is not meant to be used for error exit.
//
//------------------------------------------------------------------------------
bool 
CSimplePen::Aborted()
{
    // Ask the sink
    return m_pSink->Aborted();
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CRail
//  
//  Synopsis:
//      Stores edges belonging to one of the two "rails" (inner or outer)
//      belonging to the outline of the stroke.
//

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Extend to a new point, possibly looping thru the center. This method
//      will extend the rail to the new offset point, but if this goes against
//      the given direction of the segment, then it will go through the
//      additional point.  This handles the scenario of a sharp turn or curve
//      where the offset inside the turn goes backwards.  In that case, to avoid
//      ugly gaps, we take the outline through the center. Created:
//
//------------------------------------------------------------------------------
HRESULT 
CRail::ExtendTo(
    __in_ecount(1) const GpPointR &ptTo,
        // The target point
    __in_ecount(1) const GpPointR &vec,
        // The direction vector to test against
    __in_ecount(1) const GpPointR &pt1,
        // First point to draw to in case of gap
    __in_ecount(1) const GpPointR &pt2,
        // Second point to draw to in case of gap
    __in_ecount(1) const GpPointR &ptSpine,
        // Most recent point on the spine
    __inout_ecount(1) CShape &shape
        // Shape we are widening to
    )
{
    HRESULT hr;
    Assert(!IsEmpty());
    GpPointR ptCurrent(GetEndPoint());

    if ((ptTo - ptCurrent) * vec < 0)
    {
        // The offset is going in a direction opposite to the spine, so
        // we need to add a tile to cover the gap
        CMilPoint2F pt[4] = {GetEndPoint(),
                            CMilPoint2F(pt1),                             
                            CMilPoint2F(pt2),
                            GetEndPoint()}; 
        IFC(shape.AddLines(pt, 4));

        IFC(LineToPtR(ptSpine));
    }            

    IFC(LineToPtR(ptTo));
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Set the last point to a given value.
//
//------------------------------------------------------------------------------
HRESULT 
CRail::SetCurrentPoint(
    __in_ecount(1) const GpPointR &P
        // In: The point to set to
    )
{
    if (IsEmpty())
        return E_UNEXPECTED;

    P.Set(m_rgPoints.Last());
    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Concatenate a revered copy of another rail
//
//------------------------------------------------------------------------------
HRESULT 
CRail::ReverseJoin(
    __in_ecount(1) const CRail &other
        // In: The rail to concatenate
    )
{
    HRESULT hr = S_OK;
    UINT i;

    if (other.m_rgPoints.GetCount() < 2  ||  other.m_rgTypes.GetCount() == 0)
    {
        goto Cleanup;
    }
    
    // It is assumed that the last point of the other rail repeats the last
    // point of this rail, hence we skip it.
    i = other.m_rgPoints.GetCount() - 2;
    for ( ; ; )
    {
        IFC(m_rgPoints.Add(other.m_rgPoints[i]));
        if (0 == i)
            break;
        i--;
    }

    i = other.m_rgTypes.GetCount() - 1;
    for ( ; ; )
    {
        IFC(m_rgTypes.Add(other.m_rgTypes[i]));
        if (0 == i)
            break;
        i--;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CShapeWideningSink
//  
//  Synopsis:
//      A sink for the widener that populates a new CShape.  It maintains two
//      rails, one for each side of the widened outline.  The right rail
//      accumulates the results.
//

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CShapeWideningSink::CShapeWideningSink(
    __in_ecount(1) CShape &oShape
        // The shape receiving the results
    )
    : m_fFitCurves(false), m_oShape(oShape)
{
    m_pRail[RAIL_LEFT] = m_pRail[RAIL_RIGHT] = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor
//
//------------------------------------------------------------------------------
CShapeWideningSink::~CShapeWideningSink()
{
    delete m_pRail[RAIL_LEFT];
    delete m_pRail[RAIL_RIGHT];
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Start the rails.
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::StartWith(
    __in_ecount(2) const GpPointR *ptOffset
        // Left and right offset points
    )
{
    Assert(NULL == m_pRail[RAIL_LEFT]   &&   NULL == m_pRail[RAIL_RIGHT]);

    HRESULT hr = S_OK;

    m_pRail[RAIL_LEFT] = new CRail();
    m_pRail[RAIL_RIGHT] = new CRail();

    IFCOOM(m_pRail[RAIL_LEFT]);
    IFCOOM(m_pRail[RAIL_RIGHT]);

    IFC(m_pRail[RAIL_LEFT]->StartAtPtR(ptOffset[RAIL_RIGHT]));  // Starting the cap
    IFC(m_pRail[RAIL_RIGHT]->StartAtPtR(ptOffset[RAIL_RIGHT])); // Starting the right rail

Cleanup:    
    if (FAILED(hr))
    {
        delete m_pRail[RAIL_LEFT];
        delete m_pRail[RAIL_RIGHT];

        m_pRail[RAIL_LEFT] = m_pRail[RAIL_RIGHT] = NULL;
    }

    RRETURN(hr);

}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a pair of points to the 2 sides of the polygon - simple version
//
//      This method is called when we are widening a line segment, so we don't
//      have to guard against kinks (corners are handled separately)
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::QuadTo(
    __in_ecount(2) const GpPointR *ptOffset
        // In: Left & right offset points
    )
{
    Assert(0 == RAIL_LEFT && 1 == RAIL_RIGHT);
    HRESULT hr;

    IFC(m_pRail[RAIL_LEFT]->LineToPtR(ptOffset[RAIL_LEFT]));
    IFC(m_pRail[RAIL_RIGHT]->LineToPtR(ptOffset[RAIL_RIGHT]));

Cleanup:    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a pair of points to the 2 sides of the polygon, with testing for
//      kinks
//
//      This method is called when we are widening a curve segment; there is no
//      corner handling between segments, so on a sharp turn the offset may go
//      backwards.  We need to check for that and handle it if it happens.
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::QuadTo(
    __in_ecount(1) const GpPointR *ptOffset,
        // Left & right offset points thereof
    __in_ecount(1) const GpPointR &vecSeg,
        // Segment direction we're coming from 
    __in_ecount(1) const GpPointR &ptSpine,
        // The corresponding point on the stroke's spine
    __in_ecount(1) const GpPointR &ptSpinePrev
        // The previous point on the stroke's spine
    )
{
    Assert(0 == RAIL_LEFT && 1 == RAIL_RIGHT);
    HRESULT hr = S_OK;

    IFC(m_pRail[RAIL_LEFT]->ExtendTo(ptOffset[RAIL_LEFT],
                                vecSeg, 
                                ptSpinePrev, 
                                ptOffset[RAIL_LEFT], 
                                ptSpine, 
                                m_oShape));
    
    IFC(m_pRail[RAIL_RIGHT]->ExtendTo(ptOffset[RAIL_RIGHT], 
                                 vecSeg, 
                                 ptOffset[RAIL_RIGHT], 
                                 ptSpinePrev, 
                                 ptSpine, 
                                 m_oShape));
Cleanup:    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a Bezier curve to the given side
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::CurveWedge(
    RAIL_SIDE side,
        // Which side - RAIL_LEFT or RAIL_RIGHT
    __in_ecount(1) const GpPointR &ptBez_1,
        // First control point
    __in_ecount(1) const GpPointR &ptBez_2,
        // Second control point
    __in_ecount(1) const GpPointR &ptBez_3
        // Last point
    )
{
    RRETURN(m_pRail[side]->BezierToPtR(ptBez_1, ptBez_2, ptBez_3));
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add the 2 Bezier curves of a round cap - always to the left rail
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::BezierCap(
    __in_ecount(1) const GpPointR &,
        // Ignored here
    __in_ecount(1) const GpPointR &pt0_1,
        // First arc's first control point
    __in_ecount(1) const GpPointR &pt0_2,
        // First arc's second control point
    __in_ecount(1) const GpPointR &ptMid,
        // The point separating the 2 arcs
    __in_ecount(1) const GpPointR &pt1_1,
        // Second arc's first control point
    __in_ecount(1) const GpPointR &pt1_2,
        // Second arc's second control point
    __in_ecount(1) const GpPointR &ptEnd
        // The cap's last point
    )
{
    HRESULT hr;
    
    IFC(m_pRail[RAIL_LEFT]->BezierToPtR(pt0_1, pt0_2, ptMid));
    IFC(m_pRail[RAIL_LEFT]->BezierToPtR(pt1_1, pt1_2, ptEnd));
    
Cleanup:    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Set the values of the current points
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::SetCurrentPoints(
    __in_ecount(2) const GpPointR *P
        // In: Array of 2 points
    )
{
    HRESULT hr;
    
    IFC(m_pRail[RAIL_LEFT]->SetCurrentPoint(P[0]));
    IFC(m_pRail[RAIL_RIGHT]->SetCurrentPoint(P[1]));

Cleanup:    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Do the inner corner
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::DoInnerCorner(
    RAIL_SIDE side,
        // In: The side of the inner corner
    __in_ecount(1) const GpPointR &ptCenter,
        // In: The corner's center
    __in_ecount(2) const GpPointR *ptOffset
        // In: The offset points of new segment
    )
{
    HRESULT hr;
        
    IFC(m_pRail[side]->LineToPtR(ptCenter));
    IFC(m_pRail[side]->LineToPtR(ptOffset[side]));
    
Cleanup:    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Create a triangle endcap, adding it to the left rail
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::CapTriangle(
    __in_ecount(1) const GpPointR &ptStart,
        // Trianagle base start point
    __in_ecount(1) const GpPointR &ptApex,
        // Triangle's apex
    __in_ecount(1) const GpPointR &ptEnd
        // Trianagle base end point
    )
{
    Assert(0 == RAIL_LEFT);
    HRESULT hr;
    
    IFC(m_pRail[RAIL_LEFT]->LineToPtR(ptApex));
    IFC(m_pRail[RAIL_LEFT]->LineToPtR(ptEnd));

Cleanup:    
    RRETURN(hr);
}       

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a line which is a flat or a sqare cap to the left rail.  The cap
//      goes from one base point to the other, and the first one is already in. 
//      If this is a start cap the the line should be going from left to right,
//      and at the end it should be going from right to left.
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::CapFlat(
    __in_ecount(2) const GpPointR *ppt,
        // In: The base points
    RAIL_SIDE side
        // In: Which side the cap endpoint is
    )
{
    HRESULT hr;
    
    // Move the current point
    IFC(m_pRail[RAIL_LEFT]->SetCurrentPoint(ppt[OPPOSITE_SIDE(side)]));
    
        // Draw a line to the base's endpoint
    IFC(m_pRail[RAIL_LEFT]->LineToPtR(ppt[side]));

Cleanup:    
    RRETURN(hr);
}       

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a bunch of polygons - typically from a line shape
//
//  Notes:
//      What we really want here is a union operation.  But it's much cheaper
//      to just add the line shape's figures to our shape. The impact is that
//      the line shape designer has to be very careful about the orientation of
//      his figures, and to be aware that we won't respect alternate fill mode.
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::AddFill(
    __in_ecount(1) const CShape &oShape,
        // The shape to be filled
    __in_ecount(1) const CMILMatrix &matrix
        // Transformation
    )
{
    HRESULT hr = S_OK;
    UINT i;
    UINT count = oShape.GetFigureCount();
    CFigureData *pFigure;

    for (i = 0;  i < count;  i++)
    {
        if (oShape.GetFigure(i).IsFillable())
        {
            IFC(m_oShape.AddFigure(pFigure));
            Assert(pFigure);
            IFC(pFigure->Copy(oShape.GetFigureData(i)));
            pFigure->Transform(matrix);
        }
    }
Cleanup:
    RRETURN(hr);
}       

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add the contents of the rails to the output path as a single closed
//      figure
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::AddFigure()
{
    HRESULT hr;

    // Concatenate the two rails to form one closed figure    
    IFC(m_pRail[RAIL_LEFT]->ReverseJoin(*m_pRail[RAIL_RIGHT]));
    delete m_pRail[RAIL_RIGHT];
    m_pRail[RAIL_RIGHT] = NULL;

    // Close and add to the shape
    if (!m_pRail[RAIL_LEFT]->HasNoSegments())
    {
        IFC(m_pRail[RAIL_LEFT]->Close());
        IFC(m_oShape.AddAndTakeOwnership(m_pRail[RAIL_LEFT]));
        m_pRail[RAIL_LEFT] = NULL;   // Now owned by the shape
    }
Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Draw lines from left to right and from right to left.
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::SwitchSides()
{
    HRESULT hr;
    GpPointR ptLeft(GetCurrentPoint(RAIL_LEFT));
    
    IFC(m_pRail[RAIL_LEFT]->LineToPtR(GetCurrentPoint(RAIL_RIGHT)));
    IFC(m_pRail[RAIL_RIGHT]->LineToPtR(ptLeft));

Cleanup:    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a point to a given side
//
//------------------------------------------------------------------------------
HRESULT 
CShapeWideningSink::PolylineWedge(
    RAIL_SIDE side,
        // Which side to add to - RAIL_RIGHT or RAIL_LEFT
    UINT count,
        // Number of points
    __in_ecount(count) const GpPointR *pPoints
        // The polyline vertices
    )
{
    HRESULT hr = S_OK;

    for (UINT i = 0; i < count; i++)
    {
        IFC(m_pRail[side]->LineToPtR(pPoints[i]));
    }

Cleanup:    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSimplePen::RoundTo
//
//  Synopsis:
//      Round the corner that would have been introduced by widening a very
//      curved and very wide flattened segment
//
//------------------------------------------------------------------------------
HRESULT
CSimplePen::RoundTo(
    __in_ecount(1) const GpPointR &vecRad,
        // In: Radius vector of the outgoing segment
    __in_ecount(1) const GpPointR &ptCenter,
        // In: Corner center point
    __in_ecount(1) const GpPointR &vecIn,
        // In: Vector in the direction comin in
    __in_ecount(1) const GpPointR &vecOut
        // In: Vector in the direction going out
    )
{
    HRESULT hr;
    GpPointR vecOffset, ptNext[2];
    
    // Get the new radius vector and offset points on the outgoing segment
    GetOffsetVector(vecRad, vecOffset);
    ptNext[0] = ptCenter - vecOffset;
    ptNext[1] = ptCenter + vecOffset;
    
    // Determine the outer side of the turn
    RAIL_SIDE side = Determinant(vecIn, vecOut) > 0 ?RAIL_LEFT : RAIL_RIGHT;
    
    // Round the outer corner
    IFC(RoundCorner(ptCenter, m_ptCurrent[side], ptNext[side], vecRad, side));

    // Update for the next segment
    m_vecRad = vecRad;
    m_vecOffset = vecOffset;
    m_ptPrev = ptCenter;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CDasher
//  
//  Synopsis:
//      Adapter to a CPen that provides dashing functionality.
//
//  Notes:
//
//      DASHED LINES DESIGN
//                  
//      There are several widening scenarios.  The following crude diagrams
//      depict the flow of information:
//      
//      Simple pen without dashes: CWidener --> CSimplePen --> CWideningSink
//          
//      Simple pen with dashes: CWidener --> CDasher --> CSimplePen -->
//      CWideningSink
//              
//      Glossary An EDGE is a smooth piece of the figure between corners or
//      start and end.  The edge is a sequence of SEGMENTS.  If the edge is a
//      straight line then it comprises one segment.  If it is a curve then the
//      segments are the result of its flattening.
//      
//      CDasher accumulates segments with the information needed for widening
//      and accumulated length. At every corner (between edges) and at the
//      figure end, the Dasher flushes the segments buffer and sends the dashes
//      to the pen to draw.
//      
//      The buffer must contain all the information needed for the pen at flush
//      time, so we record points, tangents, and a flag indicating whether the
//      segment came from a line segment (rather than from curve flattening)
//      
//      If the figure is closed then the first dash may have to be the second
//      half of the last dash.  So if it starts on a dash, we'll start it with
//      a flat cap.  After the last dash we'll do the corner (between figure
//      end and start) and exit with a flat cap, that will abut with the flat
//      cap of the first dash.  If there is no end dash then we'll append a
//      0-length segment with the right cap.
//      
//      Some of the functionality of CDasher is delegated to its class members
//      CSegments, capturing the data and behavior of the segments buffer, and
//      CDashSequence, encapsulating the dash sequence.
//      
//      We dash one edge at a time. We try to dash it in a synchronized mode,
//      i.e.  always ending at the same point (=DashOffset) in the dash
//      sequence. For that we tweak the sequence length. But if the edge is
//      substantially shorter than one full instance then we dash in
//      unsynchronized mode. For the canned dash styles the offset is set to
//      half the first dash. 
//

//+-----------------------------------------------------------------------------
//
//  Struct:
//      CDasher::CSegData
//  
//  Synopsis:
//      Stores information about a given segment.
//

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CDasher::CSegData::CSegData(
    bool fIsALine,
        // In: =true if this is a line segment
    __in_ecount(1) const GpPointR &ptEnd,
        // In: Segment endpoint
    __in_ecount(1) const GpPointR &vecTangent,
        // In: The tangent vector there
    __in_ecount(1) const GpPointR &vecSeg,
        // In: The segment direction vector
    GpReal rLocation,
        // In: Accumulated length so far
    GpReal rDashScaleFactor,
        // In: Amount by which dashes will be scaled along this segment
    bool fBezierEnd
        // In: True if this is the last segment on a Bezier
    )
: m_ptEnd(ptEnd), m_vecTangent(vecTangent), m_vecSeg(vecSeg), 
  m_rLocation(rLocation), m_fIsALine(fIsALine), m_fBezierEnd(fBezierEnd),
  m_rDashScaleFactor(rDashScaleFactor)
{
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CDasher::CSegments
//  
//  Synopsis:
//      Stores the yet-to-be processed segments.
//

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize the segment buffer
//
//------------------------------------------------------------------------------
HRESULT
CDasher::CSegments::Initialize(
    __in_ecount_opt(1) const CMILMatrix *pMatrix
        // Render transform (NULL OK)
    )
{
    HRESULT hr = S_OK;

    if (pMatrix)
    {
        CMatrix22 matrix(*pMatrix);
        IFC(matrix.GetInverseQuadratic(m_rCxx, m_rCxy, m_rCyy));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Start a dash
//
//------------------------------------------------------------------------------
HRESULT 
CDasher::CSegments::StartWith(
    __in_ecount(1) const GpPointR &ptStart,
        // In: Starting point
    __in_ecount(1) const GpPointR &vecTangent
        // In: The tangent vector there
    )
{
    Assert(0 == m_rgSegments.GetCount());
    RRETURN(m_rgSegments.Add(
            CSegData(
                true,       // (ignored)
                ptStart,
                vecTangent,
                vecTangent, 
                0,
                0, // (ignored)
                false       // => Not end of a Bezier
                )));
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Accept a point on a line segment
//
//------------------------------------------------------------------------------
HRESULT 
CDasher::CSegments::Add(
    __in_ecount(1) const GpPointR &ptEnd,
        // In: Segment endpoint
    __in_ecount_opt(1) const GpPointR *pTangent,
        // Optional: Tangent vector there
    bool fBezierEnd
        // Optional: true if this is the last segment on a Bezier
    )
{
    HRESULT hr = S_OK;

    Assert(m_rgSegments.GetCount());
    GpPointR vecSeg(m_rgSegments.Last().m_ptEnd, ptEnd);

    // Get the pre-transform length of the segment
    GpReal rPreTransformLength =
        sqrt(m_rCxx * vecSeg.X * vecSeg.X + 
             m_rCxy * vecSeg.X * vecSeg.Y + 
             m_rCyy * vecSeg.Y * vecSeg.Y);

    if(rPreTransformLength >= FUZZ)
    {
        bool fIsALine = NULL == pTangent;

        GpReal rDashScaleFactor =
        sqrt(vecSeg.X * vecSeg.X + vecSeg.Y * vecSeg.Y) / rPreTransformLength;

        vecSeg /= rPreTransformLength;

        GpReal rLocation = rPreTransformLength + GetLength();

        hr = m_rgSegments.Add(
            CSegData(
                fIsALine,
                ptEnd, 
                fIsALine ? vecSeg : *pTangent,
                vecSeg,
                rLocation,
                rDashScaleFactor,
                fBezierEnd
                ));
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Reset to empty state
//
//------------------------------------------------------------------------------
void 
CDasher::CSegments::Reset()
{
    m_rgSegments.Reset();
    m_uCurrentSegment = 1;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Get the point and vectors at a given location on the current segment
//
//------------------------------------------------------------------------------
void
CDasher::CSegments::ProbeAt(
    GpReal rLoc,
        // In: The location (lengthwise) to probe at
    __out_ecount(1) GpPointR &pt,
        // Out: Point there
    __out_ecount(1) GpPointR &vecTangent,
        // Out: Tangent vector at segment end
    bool fAtSegEnd
        // In: At segment end if true
    ) const
{
    Assert(m_uCurrentSegment < m_rgSegments.GetCount());
    Assert(0 < m_uCurrentSegment);

    vecTangent = m_rgSegments[m_uCurrentSegment].m_vecTangent;

    if (fAtSegEnd    ||    rLoc > m_rgSegments[m_uCurrentSegment].m_rLocation)
    {
        //
        // This should only happen when rLoc is numerically close to m_rLocation
        //
        pt = m_rgSegments[m_uCurrentSegment].m_ptEnd;
    }
    else
    {
        rLoc -= m_rgSegments[m_uCurrentSegment-1].m_rLocation;
        if (rLoc < 0)
            rLoc = 0;

        if (!m_rgSegments[m_uCurrentSegment].m_fIsALine)
        {
            //
            // The current point is somewhere within a line segment coming from
            // a Bezier. Approximate the tangent by interpolating.
            //
            // Note: We are linearly interpolating from two points on a cubic,
            // so the interpolated vector could be significantly different than
            // the "actual" tangent vector. This, however, is beside the point.
            // All we're trying to do here is ensure continuity and consistency
            // with the figure's start and end tangent vectors.
            //

            GpReal relativeLoc = rLoc / (m_rgSegments[m_uCurrentSegment].m_rLocation
                                         - m_rgSegments[m_uCurrentSegment-1].m_rLocation);

            vecTangent = m_rgSegments[m_uCurrentSegment].m_vecTangent * relativeLoc + 
                         m_rgSegments[m_uCurrentSegment-1].m_vecTangent * (1.0 - relativeLoc );
        }

        GpPointR vecSeg = m_rgSegments[m_uCurrentSegment].m_vecSeg;
        pt = m_rgSegments[m_uCurrentSegment-1].m_ptEnd + vecSeg * rLoc;
    }
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CDasher::CDashSequence
//  
//  Synopsis:
//      Stores the sequence of dashes to apply to the stroke.
//
//  Notes:
//      All APIs take arguments in edge space (edge space records how far we
//      have travelled along an edge). Internally, all our computations are
//      done in dash space (how far along in the dash array we are). Note that
//      dash space is calculated modulo the length of the dash array, so in
//      order to convert from dash space to edge space we have to keep track of
//      how many times we've iterated over the dash array
//      (m_uCurrentIteration).
//

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CDasher::CDashSequence::CDashSequence()
    : m_uCurrentDash(1), m_rCurrentLoc(0), 
      m_rEdgeSpace0(0), m_uCurrentIteration(0), m_uStartDash(1), m_rLength(0) 
{
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Set from a dash lengths array
//
//------------------------------------------------------------------------------
HRESULT
CDasher::CDashSequence::Initialize(
    __in_ecount(1) const CPlainPen &pen
        // In: The stroking pen
    )
{
    HRESULT hr = S_OK;
    UINT count = pen.GetDashCount();
    UINT i;

    GpReal rPenWidth = max(fabs(pen.GetWidth()), fabs(pen.GetHeight()));
    GpReal rDashOffset = pen.GetDashOffset() * rPenWidth;

    if (count < 2  ||  0 != (count & 1))
    {
        IFC(E_INVALIDARG);
    }

    //
    // The working representation is an array of count+1 entries. The first
    // entry is -offset, and the rest are accumulative length from there, all
    // multiplied by pen-width.
    //

    Assert(0 == m_rgDashes.GetCount());
    IFC(m_rgDashes.AddMultiple(count+1));
    
    // Initially the dash sequence starts at 0.
    m_rgDashes[0] = 0;
    for (i = 0;   i < count;  i++)
    {
        m_rgDashes[i+1] = m_rgDashes[i] + pen.GetDash(i) * rPenWidth;
    }
    if (_isnan(m_rgDashes[count]))
    {
        IFC(WGXERR_BADNUMBER);
    }

    if (m_rgDashes[count] < MIN_DASH_ARRAY_LENGTH)
    {
        //
        // To avoid an infinite loop when rendering dashes, scale all
        // the dashes up so the dash array takes up MIN_DASH_ARRAY_LENGTH.
        //

        GpReal rScale = MIN_DASH_ARRAY_LENGTH/m_rgDashes[count];
        
        for (i = 0;   i < count-1;  i++)
        {
            // NaNs get clamped to m_rgDashes[i].
            m_rgDashes[i+1] = ClampDouble(m_rgDashes[i+1]*rScale, m_rgDashes[i], MIN_DASH_ARRAY_LENGTH);
        }

        m_rgDashes[count] = MIN_DASH_ARRAY_LENGTH;
    }
    m_rLength = m_rgDashes[count];

    // Make sure the dash offset lies within the dash-sequence interval
    if (!(0 <= rDashOffset  &&  rDashOffset < m_rLength))
    {
        rDashOffset = GpModF(static_cast<REAL>(rDashOffset), static_cast<REAL>(m_rLength));
        if (!(0 <= rDashOffset  &&  rDashOffset < m_rLength))
        {
            // GpModF failed, probably because rDashOffset is NaN
            rDashOffset = 0;
        }
    }
    Assert((0 <= rDashOffset)  &&  (rDashOffset < m_rLength));

    // Find the end of the dash/space that contains the offset
    for (m_uStartDash = 1;
         (m_uStartDash < count)  &&  (m_rgDashes[m_uStartDash] < rDashOffset);
         m_uStartDash++);

    // Now shift the dashes by the dash-offset to make 0 the starting point
    for (i = 0;   i <= count;    i++)
    {
        m_rgDashes[i] -= rDashOffset;
        Assert( (i == 0) || (m_rgDashes[i] >= m_rgDashes[i-1]) );
    }

    //
    // Ordinarily, this will get set during Dasher.BeginFigure(), but in case a
    // figure is never started and we're asked to close it (think the
    // degenerate line-segment case), we need to be prepared.
    //
    m_uCurrentDash = m_uStartDash;

    // Sanity check
    Assert(0 < m_uStartDash &&  m_uStartDash <= count);
    Assert(m_rgDashes[m_uStartDash - 1] <= 0);
    Assert(m_rgDashes[m_uStartDash] >= 0);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDasher::CDashSequence::Increment
//
//  Synopsis:
//      Increment to the next dash or gap
//
//------------------------------------------------------------------------------
void
CDasher::CDashSequence::Increment()
{
    m_rCurrentLoc = m_rgDashes[m_uCurrentDash];
    m_uCurrentDash++;

    if (m_uCurrentDash >= m_rgDashes.GetCount())
    {
        m_uCurrentDash = 1;
        m_uCurrentIteration++;
        m_rCurrentLoc = m_rgDashes[0];
    }
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Reset the dash sequence
//
//------------------------------------------------------------------------------
void
CDasher::CDashSequence::Reset()
{
    // Reset to pristine state
    m_uCurrentDash = m_uStartDash;
    m_rCurrentLoc = 0;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CDasher::CDasher(
    __in_ecount(1) CPen *pPen
        // In: The internal widening pen
    )
    :m_pPen(pPen), m_eDashCap(MilPenCap::Flat),
     m_fIsPenDown(false), m_fIsFirstCapPending(false), m_fIgnoreDash(false), m_fViewableSpecified(false)
{
    Assert(pPen);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize the dashing of a figure
//
//------------------------------------------------------------------------------
HRESULT
CDasher::Initialize(
    __in_ecount(1) const CPlainPen &pen,
        // In: The stroking pen
    __in_ecount_opt(1) const CMILMatrix *pMatrix,
        // In: Render transform (NULL OK)
    __in_ecount_opt(1) const MilRectF *prcViewableInflated
        // In: The viewable region, inflated by the stroke properties (NULL
        // OK)
    )
{
    HRESULT hr;

    m_eDashCap = pen.GetDashCap();

    if (prcViewableInflated != NULL)
    {
        m_rcViewableInflated = *prcViewableInflated;
        m_fViewableSpecified = true;
    }
    else
    {
        m_fViewableSpecified = false;
    }

    IFC(m_oDashes.Initialize(pen));

    IFC(m_oSegments.Initialize(pMatrix));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize the dashing of a figure
//
//------------------------------------------------------------------------------
HRESULT 
CDasher::StartFigure(
    __in_ecount(1) const GpPointR &pt,
        // In: Figure's firts point
    __in_ecount(1) const GpPointR &vec,
        // In: First segment's (non-zero) direction vector
    bool fClosed,
        // In: =true if we're starting a closed figure
    MilPenCap::Enum eCapType
        // In: The start cap type
    )
{
    HRESULT hr = S_OK;

    Assert(vec*vec != 0.0);

    m_oDashes.Reset();

    m_fIsPenDown = m_oDashes.IsOnDash();
    m_fIsFirstCapPending = false;

    if (m_fIsPenDown)
    {
        if (fClosed)
        {
            // The first dash will abut the last dash with flat caps on both
            m_fIsFirstCapPending = true;
            eCapType = MilPenCap::Flat;
        }
        IFC(m_pPen->StartFigure(pt, vec, false, eCapType));
    }

    IFC(m_oSegments.StartWith(pt, vec));
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Accept a point on a line segment
//
//------------------------------------------------------------------------------
HRESULT 
CDasher::AcceptLinePoint(
    __in_ecount(1) const GpPointR &point
        // In: Point to draw to
    )
{
    RRETURN(m_oSegments.Add(point));
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Accept a point on a line segment
//
//------------------------------------------------------------------------------
HRESULT 
CDasher::AcceptCurvePoint(
    __in_ecount(1) const GpPointR &point,
        // In: The point
    __in_ecount(1) const GpPointR &vecTangent,
        // In: The tangent there
    bool fLast
        // In: Is this the last point on the curve?
    )
{
    RRETURN(m_oSegments.Add(point, &vecTangent, fLast));
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a corner between 2 lines/Beziers
//
//------------------------------------------------------------------------------
HRESULT
CDasher::DoCorner(
    __in_ecount(1) const GpPointR &pt,
        // Corner center point
    __in_ecount(1) const GpPointR &vecIn,
        // Vector in the direction comin in
    __in_ecount(1) const GpPointR &vecOut,
        // Vector in the direction going out
    MilLineJoin::Enum eLineJoin,
        // Corner type
    bool fSkipped,
        // =true if this corner straddles a degenerate segment 
    bool fRound,
        // Enforce rounded corner if true
    bool fClosing
        // This is the last corner in a closed figure if true
    )
{
    HRESULT hr = S_OK;

    // Lay out the dashes on the edge that ends at this corner
    IFC(Flush(false));

    if (m_fIsPenDown)
    {
        // Let the pen draw the corner
        IFC(m_pPen->DoCorner(pt, vecIn, vecOut, eLineJoin, fSkipped, fRound, fClosing));
    }

    // Start accumlating segments on the edges that starts at this corner
    IFC(m_oSegments.StartWith(pt, vecOut));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSimplePen::EndStrokeOpen
//
//  Synopsis:
//      End an a stroke as a open
//
//------------------------------------------------------------------------------
HRESULT 
CDasher::EndStrokeOpen(
    bool fStarted,
        // Ignored here
    __in_ecount(1) const GpPointR &ptEnd,
        // Figure's endpoint
    __in_ecount(1) const GpPointR &vecEnd,
        // Direction vector there
    MilPenCap::Enum eEndCap,
        // The type of the end cap
    MilPenCap::Enum eStartCap
        // Ignored here
    )
{
    HRESULT hr;

    IFC(Flush(true));

    if (m_fIsPenDown || (!fStarted && m_oDashes.IsOnDash()))
    {
        // Let the pen cap the current dash its choice of cap
        IFC(m_pPen->EndStrokeOpen(fStarted, ptEnd, vecEnd, eEndCap, eStartCap));
    }
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSimplePen::EndStrokeClosed
//
//  Synopsis:
//      End a stroke as closed
//
//------------------------------------------------------------------------------
HRESULT 
CDasher::EndStrokeClosed(
    __in_ecount(1) const GpPointR &ptEnd,
        // Figure's endpoint
    __in_ecount(1) const GpPointR &vecEnd)
        // Direction vector there
{
    HRESULT hr;

    IFC(Flush(!m_fIsFirstCapPending));

    if (m_fIsPenDown)
    {
        if (m_fIsFirstCapPending)
        {
            //
            // The first dash is waiting with a flat start cap for the last
            // dash to abut it.
            //
            IFC(m_pPen->EndStrokeClosed(ptEnd, vecEnd));
        }
        else
        {
            //
            // The stroke must have started with a gap, so cap this dash with a
            // dash cap.
            //
            IFC(m_pPen->EndStrokeOpen(true, ptEnd, vecEnd, m_eDashCap));
        }
    }
    else if (m_fIsFirstCapPending && (MilPenCap::Flat != m_eDashCap))
    {
        // 
        // The first dash is waiting with a flat start cap for the last dash to
        // complete it but there is no last dash. The dash-cap is not flat, so
        // we need to append a 0 length dash with the correct cap to the first
        // dash.
        //
        IFC(m_pPen->StartFigure(ptEnd, vecEnd, false, m_eDashCap));
        IFC(m_pPen->EndStrokeClosed(ptEnd, vecEnd));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Start a new dash
//
//------------------------------------------------------------------------------
HRESULT 
CDasher::StartANewDash(
    GpReal rLoc,
        // In: The location
    GpReal rWorldSpaceLength,
        // In: The dash's length in world space
    bool fAtVertex
        // In: =true if we're at a segment start or end
    )
{
    Assert(!m_fIsPenDown);  // Should not be called otherwise
    HRESULT hr = S_OK;
    
    GpPointR pt, vecTangent;
    m_oSegments.ProbeAt(rLoc, pt, vecTangent, fAtVertex);
        
    if (!m_fViewableSpecified ||
        (pt.X >= m_rcViewableInflated.left - rWorldSpaceLength  &&
         pt.X <= m_rcViewableInflated.right + rWorldSpaceLength &&
         pt.Y >= m_rcViewableInflated.top - rWorldSpaceLength   &&
         pt.Y <= m_rcViewableInflated.bottom + rWorldSpaceLength ))

    {
        m_fIsPenDown = true;
        m_fIgnoreDash = false;

        IFC(m_pPen->StartFigure(pt, vecTangent, false, m_eDashCap));
    }
    else
    {
        m_fIgnoreDash = true;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Extend the current dash
//
//------------------------------------------------------------------------------
HRESULT 
CDasher::ExtendCurrentDash(
    GpReal rLoc,
        // In: The location
    bool fAtVertex
        // In: =true if we're at a segment start or end
    )
{
    HRESULT hr = S_OK;
    GpPointR pt, vecTangent;

    if (!m_fIgnoreDash)
    {
        Assert(m_fIsPenDown);   // Should not be called otherwise

        m_oSegments.ProbeAt(rLoc, pt, vecTangent, fAtVertex);
        if (m_oSegments.IsAtALine())
        {
            IFC(m_pPen->AcceptLinePoint(pt));
        }
        else
        {
            IFC(m_pPen->AcceptCurvePoint(pt, vecTangent, m_oSegments.IsAtBezierEnd()));
        }
    }
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      End the current dash
//
//------------------------------------------------------------------------------
HRESULT 
CDasher::TerminateCurrentDash(
    GpReal rLoc,
        // In: The location
    bool fAtVertex
        // In: =true if we're at a segment start or end
    )
{
    HRESULT hr = S_OK;
    GpPointR pt, vecTangent;

    if (!m_fIgnoreDash)
    {
        Assert(m_fIsPenDown);   // Should not be called otherwise

        m_oSegments.ProbeAt(rLoc, pt, vecTangent, fAtVertex);
        if (m_oSegments.IsAtALine())
        {
            IFC(m_pPen->AcceptLinePoint(pt));
        }
        else
        {
            IFC(m_pPen->AcceptCurvePoint(pt, vecTangent, true /* last point on Bezier */));
        }

        IFC(m_pPen->EndStrokeOpen(true, pt, vecTangent, m_eDashCap));
        m_fIsPenDown = false;
    }

    m_fIgnoreDash = false;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDasher::Flush
//
//  Synopsis:
//      Process the segments buffer at the end of an edge
//
//  Notes:
//      This method emits the dashes along the polygonal piece stored in the
//      segments buffer, and then empties the buffer.
//
//------------------------------------------------------------------------------
HRESULT 
CDasher::Flush(
    bool fLastEdge
        // =true if this is the figure's last edge
    )
{
    HRESULT hr = S_OK;
    bool fDone = false;
    bool fIsOnDash = m_oDashes.IsOnDash();

    if (m_oSegments.IsEmpty())
    {
        goto Cleanup;
    }

    //
    // If a corner is right on the transition boundary between dash and gap,
    // we may need to update our pen state.
    //
    if (fIsOnDash != m_fIsPenDown)
    {
        // synchronise the state of PenDown with that of IsOnDash
        if (!m_fIsPenDown)  // we should be on a dash but the pen is not down
        {
            IFC(StartANewDash(
                0.0, // rLoc
                m_oDashes.GetStep() * m_oSegments.GetCurrentDashScaleFactor(),
                false // fAtVertex
                ));
        }
        else
        {
            // we should be on a gap but the pen is down
            IFC(TerminateCurrentDash(0.0, false));
        }
    }
    
    m_oDashes.PrepareForNewEdge();
    
    do
    {
        GpReal rDashEnd = m_oDashes.GetNextEndpoint();
        GpReal rSegEnd = m_oSegments.GetCurrentEnd();

        //
        // Arbitrate the next location between dashes and segments (shorter
        // step wins).
        //
        fIsOnDash = m_oDashes.IsOnDash();
        if (m_oSegments.IsLast()  &&  (fabs(rDashEnd - rSegEnd) < MIN_DASH_ARRAY_LENGTH))
        {
            //
            // Special treatment for the case where dash and segment ends
            // coincide.
            //

            if (m_fIsPenDown)
            {
                IFC(ExtendCurrentDash(rSegEnd, true));
            }

            IFC(DoDashOrGapEndAtEdgeEnd(fLastEdge, fIsOnDash));
            break;
        }
        else if (rDashEnd > rSegEnd)
        {
            //
            // The current dash/gap goes beyond the end of the current segment, 
            // so we step to the end of the segment within the current dash/gap
            //
            if (m_fIsPenDown)
            {
                IFC(ExtendCurrentDash(rSegEnd, true));
            }

            fDone = m_oSegments.Increment();
            if (!fDone  &&  m_oSegments.IsAtALine())
            {
                IFC(m_pPen->UpdateOffset(m_oSegments.GetCurrentDirection()));
            }

            m_oDashes.AdvanceTo(rSegEnd);
        }
        else
        {
            //
            // The current segment goes beyond the end of the current dash/gap,
            // so we step to the end of the dash/gap within the current segment
            //
            if (m_fIsPenDown) // The pen is down
            {
                if (fIsOnDash) // We're at the end of a dash
                {
                    IFC(TerminateCurrentDash(rDashEnd, false));
                }
            }
            else if (!fIsOnDash) // We're at the end of a gap
            {
                IFC(StartANewDash(
                    rDashEnd,
                    m_oDashes.GetLengthOfNextDash() * m_oSegments.GetCurrentDashScaleFactor(),
                    false // fAtVertex
                    ));

            }

            m_oDashes.Increment();
        }
    }
    while (!fDone);

Cleanup:
    // Reset the buffer
    m_oSegments.Reset();

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDasher::DoDashOrGapEndAtEdgeEnd
//
//  Synopsis:
//      Handle an end of a dash/gap that coincide with the end of an edge
//
//------------------------------------------------------------------------------
HRESULT
CDasher::DoDashOrGapEndAtEdgeEnd(
    bool fLastEdge,
        // =true if this is the figure's last edge
    bool fIsOnDash
        // =true if we are on a dash
    )
{
    HRESULT hr = S_OK;
    if (fLastEdge)
    {
        // We're at a figure's end
        if (!fIsOnDash) 
        {
            //
            // We're at the end of a gap.  It is preferable to view it as the
            // start of a dash, and let the figure cap it with the pen's line
            // cap.
            //
            IFC(StartANewDash(m_oSegments.GetLength(), 0.0 /* zero-length dash */, true));
        }
    }
    else
    {
        // We're at a corner 
        if (fIsOnDash)
        {
            //
            // We're at a corner at the end of a dash.  If the dash turns the
            // corner it will terminate immediately after that.  With flat or
            // triangle dash caps that may look pretty bad, so we terminate the
            // dash here.
            //
            IFC(TerminateCurrentDash(m_oSegments.GetLength(), true));
        }
        //
        // else we're at a corner at the end of a gap. For the same reason we
        // avoid starting a new dash here, and let it happen after we turn the
        // corner.
        //

        m_oDashes.Increment();
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CLineSegment
//  
//  Synopsis:
//      Representation of a line segment.
//

//+-----------------------------------------------------------------------------
//
//  Function:
//      Construct
//
//  Synopsis:
//
//------------------------------------------------------------------------------
void 
CLineSegment::Set(
    double rStart,
        // Start parameter
    double rEnd,
        // End parameter
    __inout_ecount(1) GpPointR &ptFirst,
        // First point, transformed, possibly modified here
    __in_ecount(1) const MilPoint2F &ptLast,
        // Last point  (raw)
    __in_ecount_opt(1) const CMILMatrix *pMatrix
        // Transformation matrix (NULL OK)
    )
{
    Assert(0 <= rStart);
    Assert(rStart < rEnd);
    Assert(rEnd <= 1);

    m_ptEnd = GpPointR(ptLast, pMatrix);
    m_vecDirection = m_ptEnd - ptFirst;

    if (rEnd < 1.0)
    {
        m_ptEnd = ptFirst + m_vecDirection * rEnd;
    }

    if (rStart > 0)
    {
        ptFirst += m_vecDirection * rStart;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegment::Flatten
//
//  Synopsis:
//
//------------------------------------------------------------------------------
HRESULT 
CLineSegment::Widen(
    __out_ecount(1) GpPointR &ptEnd,
        // End point
    __out_ecount(1) GpPointR &vecEnd
        // End direction vector
    )
{
    ptEnd = m_ptEnd;
    vecEnd = m_vecDirection;
    RRETURN(m_pTarget->AcceptLinePoint(ptEnd));
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      CLineSegment tangent
//
//------------------------------------------------------------------------------
HRESULT 
CLineSegment::GetFirstTangent(
    __out_ecount(1) GpPointR &vecTangent
        // Out: Nonzero direction vector
    ) const
{
    vecTangent = m_vecDirection;
    if (m_vecDirection * m_vecDirection < m_rFuzz)
    {
        return WGXERR_ZEROVECTOR; // Error is expected. Don't record HRESULT.
    }
    else
    {
        return S_OK;
    }
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CCubicSegment
//  
//  Synopsis:
//      Representation of a cubic Bezier segment.
//

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCubicSegment::Set
//
//  Synopsis:
//      Set for widening the current segment
//
//------------------------------------------------------------------------------
void
CCubicSegment::Set(
    double rStart,
        // Start parameter
    double rEnd,
        // End parameter
    __inout_ecount(1) GpPointR &ptFirst,
        // First point, transformed, possibly modified here
    __in_ecount(3) const MilPoint2F *ppt,
        // The rest of the points (raw)
    __in_ecount_opt(1) const CMILMatrix *pMatrix
        // Transformation matrix (NULL OK)
    )
{
    Assert(ppt);
    Assert(0 <= rStart);
    Assert(rStart < rEnd);
    Assert(rEnd <= 1);
    m_oBezier.SetPoints(rStart, rEnd, ptFirst, ppt, pMatrix);
    if (rStart > 0)
    {
        ptFirst = m_oBezier.GetFirstPoint();
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCubicSegment::Widen
//
//  Synopsis:
//
//------------------------------------------------------------------------------
HRESULT
CCubicSegment::Widen(
    __out_ecount(1) GpPointR &ptEnd,
        // End point
    __out_ecount(1) GpPointR &vecEnd
        // End direction vector
    )
{
    HRESULT hr;

    // Flatten the original curve with tangents
    IFC(m_oBezier.Flatten(true));
    ptEnd = m_oBezier.GetLastPoint();
    vecEnd = m_oBezier.GetLastTangent();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Macros:
//      QUIT_IF_NEAR, QUIT_IF_INSIDE
//  
//  Synopsis:
//      These macros bail us out when m_refTester detects a hit near the
//      boundary.  But the caller - who knows whether that piece of the
//      boundary is in the interior or on the boundary of the widened contour -
//      chooses which macro to call.
//
//      The current implementation doesn't try to check if Aborted() at the
//      finest granularity. Even if it did, changes in the code could easily
//      break that.  Since we don't want to erase any reported hit when the
//      caller invokes a new test before checking if Aborted(), only positive
//      results are recorded (using "||") instead of straight assignments.
//

#define QUIT_IF_NEAR(Foo)                                           \
    {                                                               \
        IFC(Foo);                                                   \
        m_fHitNear = m_fHitNear || m_refTester.WasAborted();        \
        if (m_fHitNear)                                             \
            goto Cleanup;                                           \
    }

#define QUIT_IF_INSIDE(Foo)                                             \
    {                                                                   \
        IFC(Foo);                                                       \
        m_fHitInside = m_fHitInside || m_refTester.WasAborted();        \
        if (m_fHitInside)                                               \
            goto Cleanup;                                               \
    }

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHitTestSink
//  
//  Synopsis:
//      Widening sink that determines if a particular point lies inside the
//      stroke.
//

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTestSink::StartWith
//
//  Synopsis:
//      Record the starting offset points
//
//------------------------------------------------------------------------------
HRESULT 
CHitTestSink::StartWith(
    __in_ecount(2) const GpPointR *ptOffset
        // Starting offset points
    )
{    
    m_ptCurrent[RAIL_LEFT] = ptOffset[RAIL_RIGHT];  // Starting the cap
    m_ptCurrent[RAIL_RIGHT] = ptOffset[RAIL_RIGHT]; // Starting the right rail

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTestSink::QuadTo
//
//  Synopsis:
//      Hit test the quadrangle formed by a widened line segment
//
//------------------------------------------------------------------------------
HRESULT 
CHitTestSink::QuadTo(
    __in_ecount(2) const GpPointR *ptOffset
        // Left & right offset points
    )
{
    HRESULT hr = S_OK;

    C_ASSERT(0 == RAIL_LEFT && 1 == RAIL_RIGHT);

    // Test the edges of the quadrangle
    QUIT_IF_INSIDE(m_refTester.StartAtR(m_ptCurrent[RAIL_RIGHT]));
    QUIT_IF_INSIDE(m_refTester.DoLineR(m_ptCurrent[RAIL_LEFT]));
    QUIT_IF_NEAR(m_refTester.DoLineR(ptOffset[RAIL_LEFT]));
    QUIT_IF_INSIDE(m_refTester.DoLineR(ptOffset[RAIL_RIGHT]));
    QUIT_IF_NEAR(m_refTester.DoLineR(m_ptCurrent[RAIL_RIGHT]));
    
    m_fHitInside = m_fHitInside || (m_refTester.GetWindingNumber() != 0);

Cleanup:
    // Update the current points
    m_ptCurrent[RAIL_LEFT] = ptOffset[RAIL_LEFT];
    m_ptCurrent[RAIL_RIGHT] = ptOffset[RAIL_RIGHT];

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTestSink::CurveWedge
//
//  Synopsis:
//      Test a rounded (i.e. pie shaped) wedge at a given side, hinged at the
//      opposite side
//
//------------------------------------------------------------------------------
HRESULT 
CHitTestSink::CurveWedge(
    RAIL_SIDE side,
        // Which side - RAIL_LEFT or RAIL_RIGHT
    __in_ecount(1) const GpPointR &ptBez_1,
        // First control point
    __in_ecount(1) const GpPointR &ptBez_2,
        // Second control point
    __in_ecount(1) const GpPointR &ptBez_3
        // Last point
    )
{
    HRESULT hr = S_OK;

    // Test a pie segment centered at the opposite side
    QUIT_IF_INSIDE(m_refTester.StartAtR(m_ptCurrent[OPPOSITE_SIDE(side)]));
    QUIT_IF_INSIDE(m_refTester.DoLineR(m_ptCurrent[side]));
    QUIT_IF_NEAR(m_refTester.DoBezierR(ptBez_1, ptBez_2, ptBez_3));
    QUIT_IF_INSIDE(m_refTester.DoLineR(m_ptCurrent[OPPOSITE_SIDE(side)]));

    m_fHitInside = m_fHitInside || (m_refTester.GetWindingNumber() != 0);
    
Cleanup:
    // Update the current point
    m_ptCurrent[side] = ptBez_3;

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTestSink::BezierCap
//
//  Synopsis:
//      Test a round (start or end) cap
//
//------------------------------------------------------------------------------
HRESULT 
CHitTestSink::BezierCap(
    __in_ecount(1) const GpPointR &ptStart,
        // Cap's start point
    __in_ecount(1) const GpPointR &pt0_1,
        // First arc's first control point
    __in_ecount(1) const GpPointR &pt0_2,
        // First arc's second control point
    __in_ecount(1) const GpPointR &ptMid,
        // The point separating the 2 arcs
    __in_ecount(1) const GpPointR &pt1_1,
        // Second arc's first control point
    __in_ecount(1) const GpPointR &pt1_2,
        // Second arc's second control point
    __in_ecount(1) const GpPointR &ptEnd
        // Cap's end point
    )
{
    HRESULT hr = S_OK;

    QUIT_IF_NEAR(m_refTester.StartAtR(m_ptCurrent[RAIL_LEFT]));
    QUIT_IF_NEAR(m_refTester.DoBezierR(pt0_1, pt0_2, ptMid));
    QUIT_IF_NEAR(m_refTester.DoBezierR(pt1_1, pt1_2, ptEnd));
    QUIT_IF_INSIDE(m_refTester.DoLineR(m_ptCurrent[RAIL_LEFT]));
    
    m_fHitInside = m_fHitInside || (m_refTester.GetWindingNumber() != 0);

Cleanup:
    m_ptCurrent[RAIL_LEFT] = ptEnd;

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTestSink::SetCurrentPoints
//
//  Synopsis:
//      Test the quad that extends to a new pair of points
//
//------------------------------------------------------------------------------
HRESULT 
CHitTestSink::SetCurrentPoints(
    __in_ecount(2) const GpPointR *P
        // Array of 2 points
    )
{
    RRETURN(QuadTo(P));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTestSink::DoInnerCorner
//
//  Synopsis:
//      Replace the inner offset point with that of the new segment
//
//------------------------------------------------------------------------------
HRESULT 
CHitTestSink::DoInnerCorner(
    RAIL_SIDE side,
        // The side of the inner corner
    __in_ecount(1) const GpPointR &,
        // Ignored here
    __in_ecount(2) const GpPointR *ptOffset
        // The offset points of new segment
    )
{
    m_ptCurrent[side] = ptOffset[side];

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTestSink::CapTriangle
//
//  Synopsis:
//      Test a (start or end) triangle cap
//
//------------------------------------------------------------------------------
HRESULT 
CHitTestSink::CapTriangle(
    __in_ecount(1) const GpPointR &ptStart,
        // Trainagle base start point
    __in_ecount(1) const GpPointR &ptApex,
        // Triangle's apex
    __in_ecount(1) const GpPointR &ptEnd
        // Triangle's end point
    )
{
    HRESULT hr = S_OK;

    QUIT_IF_NEAR(m_refTester.StartAtR(m_ptCurrent[RAIL_LEFT]));
    QUIT_IF_NEAR(m_refTester.DoLineR(ptApex));
    QUIT_IF_NEAR(m_refTester.DoLineR(ptEnd));
    QUIT_IF_INSIDE(m_refTester.DoLineR(m_ptCurrent[RAIL_LEFT]));
    
    m_fHitInside = m_fHitInside || (m_refTester.GetWindingNumber() != 0);

Cleanup:
    m_ptCurrent[RAIL_LEFT] = ptEnd;

    RRETURN(hr);
}       

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTestSink::CapFlat
//
//  Synopsis:
//      Test a (start or end) flat cap
//
//------------------------------------------------------------------------------
HRESULT
CHitTestSink::CapFlat(
    __in_ecount(2) const GpPointR *pPt,
        // The base points
    RAIL_SIDE side
        // Which side the cap endpoint is
    )
{
    HRESULT hr = S_OK;
    
    QUIT_IF_NEAR(m_refTester.StartAtR(pPt[OPPOSITE_SIDE(side)]));
    QUIT_IF_NEAR(m_refTester.DoLineR(pPt[side]));

Cleanup:
    m_ptCurrent[RAIL_LEFT] = pPt[side];
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTestSink::AddFill
//
//  Synopsis:
//      Test additional geometry that annotates the stroke (line shape)
//
//------------------------------------------------------------------------------
HRESULT 
CHitTestSink::AddFill(
    __in_ecount(1) const CShape &oShape,
        // The shape to be filled
    __in_ecount(1) const CMILMatrix &matrix
        // Transformation
    )
{
    HRESULT hr;
    CMILMatrix saved = m_refTester.GetTransform();
    
    m_refTester.SetTransform(matrix);
    IFC(oShape.HitTestFiguresFill(m_refTester));
    m_refTester.SetTransform(saved);

    m_fHitNear = m_refTester.WasAborted();

    m_fHitInside = m_fHitInside || (m_refTester.GetWindingNumber() != 0);

Cleanup:
    RRETURN(hr);
}       

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTestSink::SwitchSides
//
//  Synopsis:
//      Switch left with right
//
//------------------------------------------------------------------------------
HRESULT 
CHitTestSink::SwitchSides()
{    
    HRESULT hr = S_OK;
    QUIT_IF_NEAR(m_refTester.StartAtR(m_ptCurrent[RAIL_LEFT]));
    QUIT_IF_NEAR(m_refTester.DoLineR(m_ptCurrent[RAIL_RIGHT]));

Cleanup:
    GpPointR ptTemp(m_ptCurrent[RAIL_LEFT]);
    m_ptCurrent[RAIL_LEFT] = m_ptCurrent[RAIL_RIGHT];
    m_ptCurrent[RAIL_RIGHT] = ptTemp;

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHitTestSink::PolylineWedge
//
//  Synopsis:
//      Test a polygonal wedge at the given side, hinged at the opposite side
//
//------------------------------------------------------------------------------
HRESULT 
CHitTestSink::PolylineWedge(
    RAIL_SIDE side,
        // Which side to add to - RAIL_RIGHT or RAIL_LEFT
    UINT count,
        // Number of points in the polyline
    __in_ecount(count) const GpPointR *pPoints
        // The polyline vertices
    )
{
    HRESULT hr = S_OK;

    QUIT_IF_INSIDE(m_refTester.StartAtR(m_ptCurrent[OPPOSITE_SIDE(side)]));
    QUIT_IF_INSIDE(m_refTester.DoLineR(m_ptCurrent[side]));
    for (UINT i = 0;   i < count;    i++)
    {
        QUIT_IF_NEAR(m_refTester.DoLineR(pPoints[i]));
    }
    QUIT_IF_INSIDE(m_refTester.DoLineR(m_ptCurrent[OPPOSITE_SIDE(side)]));
 
    m_fHitInside = m_fHitInside || (m_refTester.GetWindingNumber() != 0);

Cleanup:
    // Update the current point
    if (count > 0)
    {
        m_ptCurrent[side] = pPoints[count-1];
    }

    RRETURN(hr);
}




