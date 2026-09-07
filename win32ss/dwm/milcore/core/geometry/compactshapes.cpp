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
//      Implementation of CLine and CParallelogram
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CParallelogram, MILRender, "CParallelogram");
MtDefine(CRectangle, MILRender, "CRectangle");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCompactFigure::ComputeBoundsOfPoints
//
//  Synopsis:
//      Computes a rectangle tightly bounding the array of input points
//
//------------------------------------------------------------------------------
inline void 
CCompactFigure::ComputeBoundsOfPoints(
    __in_ecount(cPoints) const MilPoint2F *points,
        // Points to compute bounds of
    __in UINT cPoints,
        // Number of points
    __out_ecount(1) MilRectF &rect
        // Resultant bounds
    )
{
    Assert(points);    
    Assert(cPoints >= 1);
    bool fEncounteredNaN = false;

    // Allocate temporary stack variables for min & max to make this function
    // easier to understand, and to avoid extra address computations (e.g., rect.left)
    float rMinX, rMaxX;
    float rMinY, rMaxY;

    // Initialize the min & max with the first point
    rMinX = rMaxX = points[0].X;
    rMinY = rMaxY = points[0].Y;

    // Double bang to ensure that the low-order bit is set. Use Logical OR to avoid mixing of bitwise/logical Operations.
    fEncounteredNaN = !!_isnan(points[0].X) || !!_isnan(points[0].Y);

    // Search thru the remaining points for greater minima & maxima
    for(UINT i = 1; i < cPoints; i++)
    {
        if (points[i].X < rMinX)
        {
            rMinX = points[i].X;
        }
        else if (points[i].X > rMaxX)
        {
            rMaxX = points[i].X;
        }

        if (points[i].Y < rMinY)
        {
            rMinY = points[i].Y;
        }
        else if (points[i].Y > rMaxY)
        {
            rMaxY = points[i].Y;
        }

        // Double bang to ensure that the low-order bit is set. Use Logical OR to avoid mixing of bitwise/logical Operations.
        fEncounteredNaN = !!_isnan(points[i].X) || !!_isnan(points[i].Y) || fEncounteredNaN;
    }

    if (!fEncounteredNaN)
    {
        // Set output rectangle to bounds of the points
        rect.left = rMinX;
        rect.top = rMinY;

        rect.right = rMaxX;
        rect.bottom = rMaxY;
    }
    else
    {
        rect.left = rect.right = 
            rect.top = rect.bottom = FLOAT_QNAN;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRectangleFigure::Set
//
//  Synopsis:
//      Set to a given rect/radius
//
//------------------------------------------------------------------------------
HRESULT
CRectangleFigure::Set(
    __in_ecount(1) const MilRectF &rect,
        // The rectangle
    REAL radius)
        // The corner radius
{
    HRESULT hr = S_OK;    

    // Should not be called with an empty rectangle
    // Used !(a < b) instead of (a >= b) to avoid asserting on NaN's
    // Can't use IsEmpty(), because we don't consider a rect whose
    // left == right to be empty.
    Assert(!(rect.right < rect.left));
    Assert(!(rect.bottom < rect.top));

    m_radius = fabs(radius);
    m_fHasCorners = (m_radius == 0.0f);

    if (m_fHasCorners)
    {
        CFigureData::InitBufferWithRectanglePoints(m_pt, rect);
    }
    else
    {
        CFigureData::InitBufferWithRoundedRectanglePoints(m_pt, rect, m_radius, m_radius);
    }

    SET_COMPACT_VALID(true);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRectangleFigure::GetBounds
//
//  Synopsis:
//      Get the bounding box
//
//------------------------------------------------------------------------------
void
CRectangleFigure::GetBounds(
    __out_ecount(1) MilRectF &rect)  const // The bounds
{
    ASSERT_COMPACT_VALID;

    if (InternalIsAxisAlignedRectangle())
    {
        rect.left = m_pt[0].X;
        rect.top  = m_pt[0].Y;
        rect.right = m_pt[2].X;
        rect.bottom = m_pt[2].Y;
    }
    else
    {
        // We're a rounded rectangle

        rect.left = m_pt[0].X;
        rect.top  = m_pt[3].Y;
        rect.right = m_pt[7].X;
        rect.bottom = m_pt[11].Y;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRectangleFigure::GetParallelogramVertices
//
//  Synopsis:
//      Get the rectangle's 4 vertices
//
//------------------------------------------------------------------------------
VOID
CRectangleFigure::GetParallelogramVertices(
    __out_ecount(4)  MilPoint2F            *pVertices,   // 4 Parallelogram's vertices
    __in_ecount_opt(1)  const CMILMatrix  *pMatrix      // Transformation (optional)
    ) const
{    
    int i;

    ASSERT_COMPACT_VALID;
    Assert(pVertices);

    Assert(InternalIsAxisAlignedRectangle());

    if (pMatrix)
    {
        for (i = 0;  i < 4;  i++)
        {
            pMatrix->Transform(&m_pt[i], pVertices + i);
        }
    }
    else
    {
        for (i = 0;  i < 4;  i++)
        {
            pVertices[i] = m_pt[i];
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRectangleFigure::GetRectangleCorners
//
//  Synopsis:
//      Get two diametrically opposing corners
//
//------------------------------------------------------------------------------
void
CRectangleFigure::GetRectangleCorners(
    __out_ecount(2)  MilPoint2F *pCorners       // 2 Rectangle's corners
    ) const
{
    ASSERT_COMPACT_VALID;
    Assert(pCorners);
    Assert(InternalIsAxisAlignedRectangle());

    pCorners[0] = m_pt[0];
    pCorners[1] = m_pt[2];
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CRectangleFigure::SetToNextSegment
//
//  Synopsis:
//      Set the current segment to the next segment, unless this is the last
//      segment
//
//  Returns:
//      false if this is the last segment
//
//------------------------------------------------------------------------------
bool
CRectangleFigure::SetToNextSegment() const
{
    ASSERT_COMPACT_VALID;

    if (InternalIsAxisAlignedRectangle())
    {
        if (s_uRectNumSegments-1 == m_uCurrentSegment)
        {
            return false;
        }
    }
    else
    {
        if (s_uRoundRectNumSegments-1 == m_uCurrentSegment)
        {
            return false;
        }
    }

    m_uCurrentSegment++;
       
    return true;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRectangleFigure::GetCurrentSegmentStart
//
//  Synopsis:
//      Get the startpoint of the current segment
//
//------------------------------------------------------------------------------
__outro_ecount(1) const MilPoint2F &
CRectangleFigure::GetCurrentSegmentStart() const
{
    ASSERT_COMPACT_VALID;

    if (InternalIsAxisAlignedRectangle())
    {
        return m_pt[m_uCurrentSegment];
    }
    else
    {
        // For every line/Bezier pair, we move forward 4 points. Odd segments are
        // lines, so if we're on one, we need to add 3 points for the previous Bezier.
        return m_pt[4*(m_uCurrentSegment/2) + 3*(m_uCurrentSegment % 2)];
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRectangleFigure::GetCurrentSegment
//
//  Synopsis:
//      Get the endpoint and type of the current segment
//
//  Returns:
//      false, because no stop applies to this figure
//
//------------------------------------------------------------------------------
bool
CRectangleFigure::GetCurrentSegment(
    __out_ecount(1) BYTE            &bType,
        // Segment type (line or Bezier)
    __deref_outro_xcount((bType == MilCoreSeg::TypeLine) ? 1 : 3) const MilPoint2F *&pt
        // Line endpoint or curve 3 last points
    ) const
{
    ASSERT_COMPACT_VALID;

    if (InternalIsAxisAlignedRectangle())
    {
        bType = MilCoreSeg::TypeLine;

        if (m_uCurrentSegment < s_uRectNumSegments-1)
        {
            pt = m_pt + m_uCurrentSegment + 1;
        }
        else
        {
            pt = m_pt;
        }
    }
    else
    {
        bool fOnBezier = (m_uCurrentSegment % 2 == 0);
        bType = fOnBezier ? static_cast<BYTE>(MilCoreSeg::TypeBezier) : static_cast<BYTE>(MilCoreSeg::TypeLine);

        if (m_uCurrentSegment < s_uRoundRectNumSegments-1)
        {
            // For every line/Bezier pair, we move forward 4 points. Odd segments are
            // lines, so if we're on one, we need to add 3 points for the previous Bezier.
            pt = m_pt + 4*(m_uCurrentSegment/2) + 3*(m_uCurrentSegment % 2) + 1;
        }
        else
        {
            pt = m_pt;
        }

    }

    return false;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRectangleFigure::SetToPreviousSegment
//
//  Synopsis:
//      Set the current segment to the previous segment, unless this is the
//      first segment
//
//  Returns:
//      false if this is the first segment
//
//------------------------------------------------------------------------------
bool
CRectangleFigure::SetToPreviousSegment() const
{
    ASSERT_COMPACT_VALID;

    if (0 == m_uCurrentSegment)
    {
        return false;
    }
    
    m_uCurrentSegment--;
    return true;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRectangle::GetTightBounds
//
//  Synopsis:
//      Get the bounding box
//
//  Note:
//      This method is guaranteed to return a well-ordered rect.
//
//------------------------------------------------------------------------------
HRESULT 
CRectangle::GetTightBounds(
    __out_ecount(1) CMilRectF &rect,
    // The bounds of this shape
    __in_ecount_opt(1) const CPlainPen *pPen,
    // The pen (NULL OK but not optional)
    __in_ecount_opt(1) const CMILMatrix *pMatrix,
    // Transformation (NULL OK but not optional)
    __in double rTolerance, 
    // Error tolerance (optional)
    __in bool fRelative,
    // True if the tolerance is relative (optional)       
    __in bool fSkipHollows) const
    // If true, skip non-fillable figures when computing fill bounds (optional)       
{
    HRESULT hr = S_OK;

    if (pPen == NULL || pPen->IsSimple())
    {
        if (pMatrix == NULL || pMatrix->Is2DAxisAlignedPreserving())
        {
            //
            // In this case, rectangle corners have no effect on the geometry bounds,
            // so it's easy to fast-path.
            //

            m_figure.GetBounds(rect);

            if (pPen)
            {
                REAL halfWidth = 0.5f * pPen->GetWidth();
                REAL halfHeight = 0.5f * pPen->GetHeight();

                rect.left -= halfWidth;
                rect.right += halfWidth;
                rect.top -= halfHeight;
                rect.bottom += halfHeight;
            }

            CMILMatrix::Transform2DBoundsNullSafe(
                pMatrix,
                rect,
                rect);
        }
        else
        {
            if (pPen)
            {
                //
                // Corners may affect the bounds, so we need more complicated logic here.
                //
                // Note that we can't simply pass off to CShapeBase::GetTightBounds, because we
                // have to worry about edge cases where rectangles act differently than general shapes
                // (in particular, miters on 0-sized rects).
                //

                CShape widened;

                IFC(WidenToShape(
                        *pPen,
                        rTolerance,
                        fRelative,
                        widened,
                        pMatrix));

                IFC(widened.GetTightBounds(rect));
            }
            else
            {
                IFC(CShapeBase::GetTightBounds(
                        rect,
                        NULL /* pPen */,
                        pMatrix,
                        rTolerance,
                        fRelative,
                        fSkipHollows));
            }
        }
    }
    else
    {
        // Widening parameters are too complicated. Drop back to general widener.
        IFC(CShapeBase::GetTightBounds(
            rect,
            pPen,
            pMatrix,
            rTolerance,
            fRelative,
            fSkipHollows));
    }

    if (!rect.HasValidValues())
    {
        IFC(WGXERR_BADNUMBER);
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRectangle::WidenToShape
//
//  Synopsis:
//      Create a shape representing the stroke
//
//------------------------------------------------------------------------------
HRESULT 
CRectangle::WidenToShape(
        __in_ecount(1) const  CPlainPen &pen,
            // The pen
        __in double           rTolerance,
            // Flattening tolerance
        __in bool             fRelative,
            // True if the tolerance is relative       
        __in_ecount(1) CShape &widened,
            // The widened shape, populated here
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Transformation matrix (NULL OK)
        __in_ecount_opt(1) const CMILSurfaceRect *prcClip
            // Clip rectangle (NULL OK)
        ) const
{
    HRESULT hr = S_OK;

    if (pen.IsSimple() && pen.IsCircular())
    {
        CMilRectF rect;
        REAL radius = m_figure.GetRadius();

        // In this case, we create two rectangle figures, one large and one
        // small, for the outer and inner edges, respectively, and set the fill
        // mode to alternate.

        MilLineJoin::Enum join = pen.GetJoin();
        REAL halfWidth = 0.5f * pen.GetWidth();
        REAL halfHeight = 0.5f * pen.GetHeight();
        MilPointAndSizeF rectF;

        Assert(_isnan(halfWidth) || halfWidth >= 0.0f);
        Assert(_isnan(halfHeight) || halfHeight >= 0.0f);

        widened.SetFillMode(MilFillMode::Alternate);

        //
        // Can't use inflate routines here because we need to deal with
        // point-sized rects (e.g. (100,200,100,200) is 0-sized but distinct
        // from (0,0,0,0) ).
        //
        m_figure.GetBounds(OUT rect);
        rect.left -= halfWidth;
        rect.right += halfWidth;
        rect.top -= halfHeight;
        rect.bottom += halfHeight;

        MilRectFFromMilRectF(OUT &rectF, &rect);

        if ( (join == MilLineJoin::Round) || !InternalIsAxisAlignedRectangle() )
        {
            IFC(widened.AddRoundedRectangle(
                    rectF,
                    radius+halfWidth,
                    radius+halfHeight));
        }
        else
        {
            REAL rBevelOffset = pen.Get90DegreeBevelOffset();

            IFC(widened.AddBeveledRectangle(
                    rectF,
                    rBevelOffset));
        }

        m_figure.GetBounds(OUT rect);

        // If the inner boundary of the stroke is degenerate, don't bother drawing it.
        if (2*halfWidth < rect.Width() && 2*halfHeight < rect.Height())
        {
            rect.left += halfWidth;
            rect.right -= halfWidth;
            rect.top += halfHeight;
            rect.bottom -= halfHeight;

            MilRectFFromMilRectF(OUT &rectF, &rect);

            // Note that this same statement works for mitered, beveled and rounded rectangles
            IFC(widened.AddRoundedRectangle(
                    rectF,
                    ClampMinFloat(radius-halfWidth, 0.0f),
                    ClampMinFloat(radius-halfHeight, 0.0f)));
        }

        // This is a no-op if pMatrix is NULL or identity
        widened.Transform(pMatrix);
    }
    else
    {
        // Widening parameters are too complicated. Drop back to general widener.
        IFC(CShapeBase::WidenToShape(
                pen,
                rTolerance,
                fRelative,
                widened,
                pMatrix,
                prcClip));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CParallelogram::Set
//
//  Synopsis:
//      Set to a given rect
//
//------------------------------------------------------------------------------
void
CParallelogramFigure::Set(
    __in_ecount(1) const MilRectF &rect)                // The rectangle
{
    // Should not be called with an empty rectangle
    // Used !(a < b) instead of (a >= b) to avoid asserting on NaN's
    Assert(!(rect.right < rect.left));
    Assert(!(rect.bottom < rect.top));

    m_pt[0].X = m_pt[3].X = rect.left;
    m_pt[0].Y = m_pt[1].Y = rect.top;
    m_pt[1].X = m_pt[2].X = rect.right;
    m_pt[2].Y = m_pt[3].Y = rect.bottom;
    
    m_uCurrentSegment = 0;

    SET_COMPACT_VALID(true);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CParallelogram::Set
//
//  Synopsis:
//      Set to a given parallelogram
//
//------------------------------------------------------------------------------
void
CParallelogramFigure::Set(
    __in_ecount(1) const CParallelogramFigure &other, // Another parallelogram
    __in_ecount_opt(1) const CBaseMatrix *pMatrix// Transformation (optional)
    )
{
    #if DBG
    Assert(other.m_fDbgValid);
    #endif

    if (pMatrix)
    {
        for (UINT i = 0;  i < 4;  i++)
        {
            pMatrix->Transform(
                &other.m_pt[i],
                OUT &m_pt[i]
                );
        }
    }
    else
    {
        RtlCopyMemory(
            OUT m_pt,
            other.m_pt,
            sizeof(m_pt)
            );
    }

    SET_COMPACT_VALID(true);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CParallelogramFigure::GetParallelogramVertices
//
//  Synopsis:
//      Get the parallelograms's 4 vertices
//
//------------------------------------------------------------------------------
VOID
CParallelogramFigure::GetParallelogramVertices(
    __out_ecount(4)  MilPoint2F            *pVertices,   // 4 Parallelogram's vertices
    __in_ecount_opt(1)  const CMILMatrix  *pMatrix      // Transformation (optional)
    ) const
{    
    int i;

    ASSERT_COMPACT_VALID;
    Assert(pVertices);

    if (pMatrix)
    {
        for (i = 0;  i < 4;  i++)
        {
            pMatrix->Transform(&m_pt[i], pVertices + i);
        }
    }
    else
    {
        for (i = 0;  i < 4;  i++)
        {
            pVertices[i] = m_pt[i];
        }
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CParallelogramFigure::SetToNextSegment
//
//  Synopsis:
//      Set the current segment to the next segment, unless this is the last
//      segment
//
//  Returns:
//      false if this is the last segment
//
//------------------------------------------------------------------------------
bool
CParallelogramFigure::SetToNextSegment() const
{
    if (3 == m_uCurrentSegment)
    {
        return false;
    }

    m_uCurrentSegment++;
    return true;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CParallelogramFigure::GetCurrentSegment
//
//  Synopsis:
//      Get the endpoint and type of the current segment
//
//  Returns:
//      false, because no stop applies to this figure
//
//------------------------------------------------------------------------------
bool
CParallelogramFigure::GetCurrentSegment(
    __out_ecount(1) BYTE            &bType,
        // Segment type (line or Bezier)
    __deref_outro_xcount((bType == MilCoreSeg::TypeLine) ? 1 : 3) const MilPoint2F *&pt
        // Line endpoint or curve 3 last points
    ) const
{
    ASSERT_COMPACT_VALID;

    bType = MilCoreSeg::TypeLine;

    if (m_uCurrentSegment < 3)
    {
        pt = m_pt + m_uCurrentSegment + 1;
    }
    else
    {
        pt = m_pt;
    }

    return false;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CParallelogramFigure::SetToPreviousSegment
//
//  Synopsis:
//      Set the current segment to the previous segment, unless this is the
//      first segment
//
//  Returns:
//      false if this is the first segment
//
//------------------------------------------------------------------------------
bool
CParallelogramFigure::SetToPreviousSegment() const
{
    if (0 == m_uCurrentSegment)
    {
        return false;
    }
    
    m_uCurrentSegment--;
    return true;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CParallelogramFigure::Transform
//
//  Synopsis:
//      Transform this rectangle
//
//------------------------------------------------------------------------------
void
CParallelogramFigure::Transform(
    __in_ecount_opt(1) const CBaseMatrix *pMatrix)  // Transformation matrix (NULL OK)
{
    ASSERT_COMPACT_VALID;

    if (NULL != pMatrix  &&  !pMatrix->IsIdentity())
    {
        pMatrix->Transform(m_pt, m_pt, 4);
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CParallelogramFigure::GetBounds
//
//  Synopsis:
//      Get the bounding box
//
//------------------------------------------------------------------------------
void
CParallelogramFigure::GetBounds(
    __out_ecount(1) MilRectF &rect) const // The bounds
{
    ASSERT_COMPACT_VALID;

    ComputeBoundsOfPoints(
        m_pt,
        ARRAY_SIZE(m_pt),
        rect
        );
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CParallelogramFigure::GetRectangleCorners
//
//  Synopsis:
//      Get two diametrically opposing corners
//
//------------------------------------------------------------------------------
void
CParallelogramFigure::GetRectangleCorners(
    __out_ecount(2)  MilPoint2F *pCorners       // 2 Rectangle's corners
    ) const
{
    ASSERT_COMPACT_VALID;
    Assert(pCorners);
    Assert(InternalIsAxisAlignedRectangle());

    pCorners[0] = m_pt[0];
    pCorners[1] = m_pt[2];
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CParallelogramFigure::Contains
//
//  Synopsis:
//      Returns true if this parallelogram fully contains the given
//      parallelogram.
//
//      The tolerance given, if positive, will effectively expand this to do the
//      comparison. If negative, it will effecively shrink this to do the
//      comparison
//
//  Note:
//      The notion of "emptiness" is not taken into consideration. The incoming
//      parallelogram is just treated as four points. If those four points have
//      no area, but all lie outside of 'this', we will return false.
//

bool CParallelogramFigure::Contains(
    __in_ecount(1) const CParallelogramFigure &other,
    float rTolerance
    ) const
{
    ASSERT_COMPACT_VALID;

    if (InternalIsAxisAlignedRectangle())
    {
        //
        // No fancy math needed- optimize away
        //
        MilRectF rcThis;
        GetAsRectangle(rcThis);

        for (UINT i = 0; i < ARRAY_SIZE(other.m_pt); i++)
        {                
            if (   other.m_pt[i].X < rcThis.left - rTolerance
                || other.m_pt[i].X > rcThis.right + rTolerance
                || other.m_pt[i].Y < rcThis.top - rTolerance
                || other.m_pt[i].Y > rcThis.bottom + rTolerance
                )
            {
                return false;
            }
        }

        return true;
    }
    else if (RtlEqualMemory(m_pt, other.m_pt, sizeof(m_pt)))
    {
        //
        // If the two shapes are equal, this contains the other.
        // We special case this case for performance.
        //
        return true;
    }
    
    //
    // The following algorithm works by testing each the four points in the other figure
    // to see if they lie within the parallelogram. Let us call each of these points T
    // and the four points of the parallelogram P0-P3
    //

    //                     P2
    //                      *                 
    //                    *  *                
    //                  *     *               
    //                *        *              
    //              *           *            
    //            *              *          
    //          *       T         *       
    //     P3 *         o          * P1   
    //         *                 *          
    //          *              *             
    //           *           *                  
    //            *        *                    
    //             *     *                      
    //              *  *                        
    //               *                          
    //              P0
    //
    
    //
    // The point T is inside the paralleogram if it is
    // 1. between P0 -> P1 and P3 -> P2
    // 2. between P1 -> P2 and P0 -> P3
    //

    //
    // To calculate if (1) is true, we let N1 = a normal vector to P0 -> P1. Note this
    // N1 is also normal to P3 -> P2 because this is a parallelogram.
    //
    // The dot product of N1 and any point T gives the signed distance to the origin
    // of a line through T & parallel to P0-P1.  Therefore to find out if a point T is
    // between the parallel lines A & B we can just dot T with N1 & see if the result is
    // between the dot of (points on) A and B with N1.
    //
    // T is then between P0 -> P1 and P3 -> P2 if
    //    min( dot(P0, N1), dot(P2, N1) ) <= dot(T, N1) <= max( dot(P0, N1), dot(P2, N1) )
    //
    // Similarly, to calculate (2), let N2 = a normal vector to P1 -> P2 and use
    //    min( dot(P0, N2), dot(P2, N2) ) <= dot(T, N2) <= max( dot(P0, N2), dot(P2, N2) )
    //
    

    {
        //
        // Calculate N1 and N2
        //
        CMilPoint2F N1 = m_pt[1] - m_pt[0];
        N1.TurnRight();
        float N1_length = N1.Norm();

        CMilPoint2F N2 = m_pt[2] - m_pt[1];
        N2.TurnRight();
        float N2_length = N2.Norm();

        //
        // Calculate the four dot products we will need
        //
        FLOAT dotP0_N1 = (N1 * m_pt[0]);
        FLOAT dotP2_N1 = (N1 * m_pt[2]);
        FLOAT dotP0_N2 = (N2 * m_pt[0]);
        FLOAT dotP2_N2 = (N2 * m_pt[2]);

        //
        // calculate the mins and maxes
        //

        FLOAT min1; // minimum value needed to calculate (1) in the description above
        FLOAT max1; // maximum value needed to calculate (1) in the description above
        FLOAT min2; // minimum value needed to calculate (2) in the description above   
        FLOAT max2; // maximum value needed to calculate (2) in the description above   
        
        if (dotP0_N1 <= dotP2_N1)
        {
            min1 = dotP0_N1;
            max1 = dotP2_N1;
        }
        else
        {
            min1 = dotP2_N1;
            max1 = dotP0_N1;
        }
        
        if (dotP0_N2 <= dotP2_N2)
        {
            min2 = dotP0_N2;
            max2 = dotP2_N2;
        }
        else
        {
            min2 = dotP2_N2;
            max2 = dotP0_N2;
        }

        //
        // Calculate tolerances for the two comparisons.
        // The dot products we do with N magnify our numbers by the
        // length of N.
        //

        float rTol1 = N1_length * rTolerance;
        float rTol2 = N2_length * rTolerance;

        //
        // For each of the points, alculate the dot product with each of
        // the normals, N1-N2
        //
        // Compare these dot products against min1 and min2 to see if each point
        // lies within the parallelogram
        //

        for (UINT i = 0; i < ARRAY_SIZE(other.m_pt); i++)
        {
            // other.m_pt[i] == T
            FLOAT dotT_N1 = N1 * other.m_pt[i];
            FLOAT dotT_N2 = N2 * other.m_pt[i];
            
            //
            // Notice similarity to optimized expression above
            //        other.m_pt[i].X < rcThis.left - rTolerance 
            //     || other.m_pt[i].X > rcThis.right + rTolerance
            //     || other.m_pt[i].Y < rcThis.top - rTolerance
            //     || other.m_pt[i].Y > rcThis.bottom + rTolerance
            //
            if (  dotT_N1 < min1 - rTol1
                || dotT_N1 > max1 + rTol1
                || dotT_N2 < min2 - rTol2
                || dotT_N2 > max2 + rTol2
              )
            {
                return false;
            }
        }
    }

    return true;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineFigure::Transform
//
//  Synopsis:
//      Transform this line
//
//------------------------------------------------------------------------------
void
CLineFigure::Transform(
    __in_ecount_opt(1) const CMILMatrix *pMatrix)  // Transformation matrix (NULL OK)
{
    ASSERT_COMPACT_VALID;

    if (NULL != pMatrix  &&  !pMatrix->IsIdentity())
    {
        pMatrix->Transform(&m_pt[START_POINT], &m_pt[START_POINT]);
        pMatrix->Transform(&m_pt[END_POINT], &m_pt[END_POINT]);
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineFigure::GetBounds
//
//  Synopsis:
//      Get the bounding box
//
//------------------------------------------------------------------------------
void
CLineFigure::GetBounds(
    __out_ecount(1) MilRectF &rect)  const // The bounds
{
    ASSERT_COMPACT_VALID;

    ComputeBoundsOfPoints(
        m_pt,
        ARRAY_SIZE(m_pt),
        rect
        );    
}



