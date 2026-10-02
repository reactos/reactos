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
//      The implementation of CFigure and its helper classes
//
//      CFigure captures the most general type of figure (sub-path)
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#pragma optimize("t", on)

MtDefine(CLineCurve, MILRender, "CLineCurve");
MtDefine(CCubicCurve, MILRender, "CCubicCurve");
MtDefine(CFigureData, MILRender, "CFigureData");
MtDefine(CFigure, MILRender, "CFigure");

const WORD FigureFlagClosed =      0x0001;
const WORD FigureFlagCurved =      0x0008;
const WORD FigureFlagRectangle =   0x0010;

// Fill & stroke flags
const WORD FigureFlagNoFill =       0x0100;
const WORD FigureFlagGapState =     0x0200;
const WORD FigureFlagHasGaps =      0x0400;

// Construction tracing flag
#if DBG
bool g_fTraceFigureConstruction = false;

#define DUMP_XY(s,x,y) DumpXY((s),(x),(y))

void DumpXY(
    IN PCWSTR pStr,     // String to dump
    IN double x,        // X coordinate
    IN double y)        // Y coordinate
{
    if (g_fTraceFigureConstruction)
    {
        OutputDebugString(pStr);
        MILDebugOutput(L"(%f, %f)\n", x, y);
    }
}
#else
#define DUMP_XY(s,x,y)
#endif

///////////////////////////////////////////////////////////////////////////////
// Implementation of CFigureData

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::StartAt
//
//  Synopsis:
//      Set the starting point of the figure
//
//------------------------------------------------------------------------------
HRESULT
CFigureData::StartAt(
    IN REAL x, REAL y)  // Figure's start point
{
    Assert(IsEmpty()); // Should only be called on an empty stomach !!
    DUMP_XY(L"StartAt ", x, y);

    MilPoint2F pt = {x, y};

    return AddPoint(pt);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::LineTo
//
//  Synopsis:
//      Add a line to the figure from the current point
//
//------------------------------------------------------------------------------
HRESULT
CFigureData::LineTo(
    IN   REAL x,         // Line's end point X
    IN   REAL y,         // Line's end point Y
    bool fSmoothJoin     // = true if forcing a smooth join
    )
{
    MilPoint2F pt = {x, y};
    DUMP_XY(L"LineTo ", x, y);

    // Should not be added to a closed or empty figure
    HRESULT hr = S_OK;
    if (IsEmpty()  ||  IsClosed())
    {
        IFC(E_UNEXPECTED);
    }
  
    // Add the line
    IFC(AddPoint(pt));
    IFC(AddAndSetTypes(1, MilCoreSeg::TypeLine, fSmoothJoin));

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::Close
//
//  Synopsis:
//      Close the current figure, adding a line segment if start != end
//
//  Notes:
//      We won't try to guess what size gap is considered 0 by the application.
//      A line segment will be added unless the start and end point are equal
//      EXACTLY.
//
//------------------------------------------------------------------------------
HRESULT
CFigureData::Close()
{
    HRESULT hr = S_OK;

    if ((m_rgPoints.GetCount() > 1) && !IsClosed())
    {
        MilPoint2F ptStart(GetStartPoint());
        MilPoint2F ptEnd(GetEndPoint());
        if ((ptStart.X != ptEnd.X)  ||  (ptStart.Y != ptEnd.Y))
        {
            IFC(LineTo(ptStart.X, ptStart.Y));
        }
        SetClosed();
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::SetToFirstSegment
//
//  Synopsis:
//      Set the current segment to be the first segment
//
//  Return:
//      True if successfully set
//
//------------------------------------------------------------------------------
bool
CFigureData::SetToFirstSegment() const
{
    bool fSet = (m_rgTypes.GetCount() >= 1);
    if (fSet)
    {
        m_uCurrentSegment = 0;
        m_uCurrentPoint = 1;
        Assert(m_uCurrentPoint < m_rgPoints.GetCount());
    }

    return fSet; 
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::SetToNextSegment
//
//  Synopsis:
//      Move to the next segment
//
//  Return:
//      True if successfully set
//
//------------------------------------------------------------------------------
bool
CFigureData::SetToNextSegment() const
{
    bool fSet = (m_uCurrentSegment < m_rgTypes.GetCount() - 1);

    if (fSet)
    {
        if (MilCoreSeg::TypeLine == GetType(m_uCurrentSegment))
        {
            m_uCurrentPoint++;
        }
        else
        {
            m_uCurrentPoint += 3;
        }
        m_uCurrentSegment++;
            
        Assert(MilCoreSeg::TypeLine == GetType(m_uCurrentSegment) && 
               m_uCurrentPoint < m_rgPoints.GetCount()
               ||
               MilCoreSeg::TypeBezier == GetType(m_uCurrentSegment) && 
               m_uCurrentPoint < m_rgPoints.GetCount() - 2);
   }

   return fSet;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::SetToLastSegment
//
//  Synopsis:
//      Set the current segment to be the last segment
//
//  Return:
//      True if successfully set
//
//------------------------------------------------------------------------------
bool
CFigureData::SetToLastSegment() const
{
#ifdef LINE_SHAPES_ENABLED
    bool fSet = (m_rgTypes.GetCount() > 0);

    if (fSet)
    {
        m_uCurrentSegment = m_rgTypes.GetCount() - 1;
        if (MilCoreSeg::TypeLine == GetType(m_uCurrentSegment))
        {
            Assert(m_rgPoints.GetCount() > 0);
            m_uCurrentPoint = m_rgPoints.GetCount() - 1;
        }
        else
        {
            Assert(m_rgPoints.GetCount() >= 3);
            m_uCurrentPoint = m_rgPoints.GetCount() - 3;
        }
    }

   return fSet;
#else
   RIP("Invalid call");
   return false;
#endif // LINE_SHAPES_ENABLED
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::SetToPreviousSegment
//
//  Synopsis:
//      Move to the previous segment
//
//  Return:
//      True if successfully set
//
//------------------------------------------------------------------------------
bool
CFigureData::SetToPreviousSegment() const
{
#ifdef LINE_SHAPES_ENABLED
    bool fSet = (m_uCurrentSegment > 0);

    if (fSet)
    {
        m_uCurrentSegment--;
        if (MilCoreSeg::TypeLine == GetType(m_uCurrentSegment))
        {
            Assert(m_uCurrentPoint > 1);
            m_uCurrentPoint--;
        }
        else
        {
            Assert(MilCoreSeg::TypeBezier == GetType(m_uCurrentSegment));
            Assert(m_uCurrentPoint > 3);

            m_uCurrentPoint -= 3;
        }
    }

   return fSet;
#else
   RIP("Invalid call");
   return false;
#endif // LINE_SHAPES_ENABLED
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::GetCurrentSegment
//
//  Synopsis:
//      Get the current segment points and type
//
//  Returns:
//      true if this is the segment where stop has been set
//
//------------------------------------------------------------------------------
bool
CFigureData::GetCurrentSegment(
    __out_ecount(1) BYTE            &bType,
        // Segment type (line or Bezier)
    __deref_outro_xcount((bType == MilCoreSeg::TypeLine) ? 1 : 3) const MilPoint2F *&pt
        // Line endpoint or curve 3 last points
    ) const
{
    bType = GetType(m_uCurrentSegment);
    pt = &m_rgPoints[m_uCurrentPoint];

    return m_uCurrentSegment >= m_uStop;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::IsAxisAlignedRectangle
//
//  Synopsis:
//      Return true if this figure is an axis aligned rectangle
//
//  Notes:
//      Returns false if a NaN is encountered.
//
//------------------------------------------------------------------------------
bool
CFigureData::IsAxisAlignedRectangle() const
{
    Assert(IsEmpty()    ||
            !IsClosed() || 
            (
             GetStartPoint().X == GetEndPoint().X && 
             GetStartPoint().Y == GetEndPoint().Y
            ) ||
            // Ignore NaNs
            _isnan(GetStartPoint().X) ||
            _isnan(GetStartPoint().Y) ||
            _isnan(GetEndPoint().X) ||
            _isnan(GetEndPoint().Y)
        );

    // We are an Axis Aligned rectangle if we are closed, have 5 points, 4
    // segments, and the points align
    return (IsClosed() && 
            m_rgPoints.GetCount() == 5 &&
            m_rgTypes.GetCount() == 4 &&
            (
                (
                (m_rgPoints[0].Y == m_rgPoints[1].Y) &&
                (m_rgPoints[0].X == m_rgPoints[3].X) &&
                (m_rgPoints[1].X == m_rgPoints[2].X) &&
                (m_rgPoints[2].Y == m_rgPoints[3].Y)
                ) ||
                (
                (m_rgPoints[0].X == m_rgPoints[1].X) &&
                (m_rgPoints[0].Y == m_rgPoints[3].Y) &&
                (m_rgPoints[1].Y == m_rgPoints[2].Y) &&
                (m_rgPoints[2].X == m_rgPoints[3].X)
                )
            )
            );
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::GetAsRectangle
//
//  Synopsis:
//      Gets the rectangle of an rectangle figure.
//
//  Notes:
//      The rect returned may not be well ordered.
//
//------------------------------------------------------------------------------

void CFigureData::GetAsRectangle(
    __out_ecount(1) MilRectF &rect           // Out: The rectangle
    ) const
{
    Assert(IsAxisAlignedRectangle());

    rect.left = m_rgPoints[0].X;
    rect.top = m_rgPoints[0].Y;

    rect.right = m_rgPoints[2].X;
    rect.bottom = m_rgPoints[2].Y;

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::GetParallelogramVertices
//
//  Synopsis:
//      This figure is a paralleogram, get its 4 vertices
//
//------------------------------------------------------------------------------
void
CFigureData::GetParallelogramVertices(
    __out_ecount_full(4) MilPoint2F *pVertices,
        // 4 Rectangle's vertices
    __in_ecount_opt(1) const CMILMatrix   *pMatrix
        // Transformation (optional)
    ) const
{
    int i;

    Assert(pVertices);
    Assert(IsAParallelogram());

    if (pMatrix)
    {
        for (i = 0;  i < 4;  i++)
        {
            pMatrix->Transform(&m_rgPoints[i], pVertices + i);
        }
    }
    else
    {
        for (i = 0;  i < 4;  i++)
        {
            pVertices[i] = m_rgPoints[i];
        }
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::GetRectangleCorners
//
//  Synopsis:
//      Get the 2 corners of this figure, presuming it's a rectangle
//
//------------------------------------------------------------------------------
void
CFigureData::GetRectangleCorners(
    __out_ecount_full(2) MilPoint2F *pVertices
        // 2 Rectangle's corners
    ) const
{
    Assert(IsAxisAlignedRectangle());

    pVertices[0] = m_rgPoints[0];
    pVertices[1] = m_rgPoints[2];
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::GetAsWellOrderedRectangle
//
//  Synopsis:
//      Gets the rectangle of an rectangle figure. This rectangle is guaranteed
//      to have positive width and height.
//
//------------------------------------------------------------------------------

void CFigureData::GetAsWellOrderedRectangle(
    __out_ecount(1) MilRectF &rect           // Out: The rectangle
    ) const
{
    Assert(IsAxisAlignedRectangle());

    if (m_rgPoints[2].X < m_rgPoints[0].X)
    {
        rect.left = m_rgPoints[2].X;
        rect.right = m_rgPoints[0].X;
    }
    else
    {
        rect.left = m_rgPoints[0].X;
        rect.right = m_rgPoints[2].X;
    }

    if (m_rgPoints[2].Y < m_rgPoints[0].Y)
    {
        rect.top = m_rgPoints[2].Y;
        rect.bottom = m_rgPoints[0].Y;
    }
    else
    {
        rect.top = m_rgPoints[0].Y;
        rect.bottom = m_rgPoints[2].Y;
    }

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::SetFrom
//
//  Synopsis:
//      Initialize this figure from another figure.
//
//  Notes:
//      Use Copy if you know that the other shape is implemented as CFigureData,
//      it is much more efficient.
//
//------------------------------------------------------------------------------

HRESULT
CFigureData::SetFrom(
    __in_ecount(1) const IFigureData   &other,      // The data to copy
    __in_ecount_opt(1) const CMILMatrix *pMatrix)   // Transformation to apply to the input
{
    HRESULT hr = S_OK;
    BYTE bType;
    const MilPoint2F *pPt;
    MilPoint2F pTransformed[3];

    // Set figure attributes
    if (other.IsAParallelogram())
        m_bFlags |= FigureFlagRectangle;

    if (!other.IsFillable())
        m_bFlags |= FigureFlagNoFill;

    // Set points and types
    m_rgPoints.Reset(false);
    m_rgTypes.Reset(false);

    if (other.IsEmpty())
        goto Cleanup;

    // Start point
    if (pMatrix)
    {
        pMatrix->Transform(&other.GetStartPoint(), pTransformed);
        IFC(m_rgPoints.AddMultipleAndSet(pTransformed, 1));
    }
    else
    {
        IFC(m_rgPoints.Add(other.GetStartPoint()));
    }

    if (!other.SetToFirstSegment())
        goto Cleanup;

    // Traverse the segments
    do
    {
        // Get the segment
        other.GetCurrentSegment(bType, pPt);
        if (MilCoreSeg::TypeLine == bType)
        {
            // It's a line segment
            if (pMatrix)
            {
                pMatrix->Transform(pPt, pTransformed);
                IFC(m_rgPoints.AddMultipleAndSet(pTransformed, 1));
            }
            else
            {
                IFC(m_rgPoints.AddMultipleAndSet(pPt, 1));
            }
        }
        else
        {
            // It's a Bezier segment
            Assert(MilCoreSeg::TypeBezier == bType);
            if (pMatrix)
            {
                pMatrix->Transform(pPt, pTransformed, 3);
                IFC(m_rgPoints.AddMultipleAndSet(pTransformed, 3));
            }
            else
            {
                IFC(m_rgPoints.AddMultipleAndSet(pPt, 3));
            }
            m_bFlags |= FigureFlagCurved;
        }

        // Set segment attributes
        if (other.IsAtASmoothJoin())
        {
            bType |= MilCoreSeg::SmoothJoin;
        }

        if (other.IsAtAGap())
        {
            bType |= MilCoreSeg::IsAGap;
            m_bFlags |= FigureFlagHasGaps;
        }

        // Add segment
        IFC(m_rgTypes.AddAndSet(1, bType));
    }
    while (other.SetToNextSegment());

    if (other.IsClosed())
    {
        SetClosed();
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::Copy
//
//  Synopsis:
//      Copy figure from another figure.
//
//------------------------------------------------------------------------------

HRESULT
CFigureData::Copy(
    __in_ecount(1) const CFigureData &other)  // The data to copy
{
    HRESULT hr = S_OK;
    m_rgPoints.Reset((INT)m_rgPoints.GetCapacity() > 2*other.m_rgPoints.GetCount());
    m_rgTypes.Reset((INT)m_rgTypes.GetCapacity() > 2*other.m_rgTypes.GetCount());
    m_bFlags = other.m_bFlags;

    IFC(m_rgPoints.Copy(other.m_rgPoints));
    IFC(m_rgTypes.Copy(other.m_rgTypes));

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::AddAndSetTypes
//
//  Synopsis:
//      Add a prescribed number of segment types
//
//------------------------------------------------------------------------------
HRESULT
CFigureData::AddAndSetTypes(
    IN int             count,   // The number of types to add
    IN MilCoreSeg::Flags type,    // Their values   
    IN bool            fSmooth) // Enforce smoothness at corner if true
{
    BYTE bType = static_cast<BYTE>(type);

    if (0 != (FigureFlagGapState & m_bFlags))
    {
        m_bFlags |= FigureFlagHasGaps;
        bType |= MilCoreSeg::IsAGap;
    }

    if (fSmooth)
    {
        bType |= MilCoreSeg::SmoothJoin;
    }

    return m_rgTypes.AddAndSet(count, bType);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::AddSegments
//
//  Synopsis:
//      Private utility for adding a bunch of segments of a given type
//
//  Notes:
//      This function does NOT check consistency between the number of points
//      and the number of segments.
//
//------------------------------------------------------------------------------
HRESULT
CFigureData::AddSegments(
    __in int                             cPoints,   // Number of points to add
    __in int                             cSegs,     // Number of segment to add
    __in_ecount(cPoints) const MilPoint2F *pPt,      // Array of points to add
    IN MilCoreSeg::Flags                   eSegType,  // The type of segments
    IN bool                              fSmooth)   // Enforce smoothness at joins if true
{
    HRESULT hr = S_OK;

    if (cPoints > 0)
    {
        Assert( 
                  (MilCoreSeg::TypeLine == eSegType && cPoints >= cSegs)
              ||
                  (MilCoreSeg::TypeBezier == eSegType && cPoints >= 3 * cSegs)
              );

        // Add the points
        IFC(m_rgPoints.AddMultipleAndSet(pPt, cPoints));

        // Add the types and set them to the given type
        hr = THR(AddAndSetTypes(cSegs, eSegType, fSmooth));
    }        

Cleanup:    
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::Reverse
//
//  Synopsis:
//      Reverse the orientation of this figure
//
//------------------------------------------------------------------------------
void
CFigureData::Reverse()
{
    // Reverse the points
    int nLast = m_rgPoints.GetCount();
    int nHalf = nLast / 2;
    nLast--;
    
    for (int i = 0;    i < nHalf;    i++)
    {
        MilPoint2F P = m_rgPoints[i];
        m_rgPoints[i] = m_rgPoints[nLast - i];
        m_rgPoints[nLast - i] = P;
    }

    // Reverse the types
    nLast = m_rgTypes.GetCount();
    nHalf = nLast / 2;
    nLast--;
    
    for (int i = 0;    i < nHalf;   i++)
    {
        BYTE b = m_rgTypes[i];
        m_rgTypes[i] = m_rgTypes[nLast - i];
        m_rgTypes[nLast - i] = b;
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::Transform
//
//  Synopsis:
//      Transform this figure
//
//------------------------------------------------------------------------------

void 
CFigureData::Transform(
    __in_ecount(1) const CBaseMatrix &matrix)
        // Transformation matrix (NULL OK)
{
    MilPoint2F *rgPoints = m_rgPoints.GetDataBuffer();
    matrix.Transform(rgPoints, rgPoints, m_rgPoints.GetCount());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::LinesTo
//
//  Synopsis:
//      Add a polygonal piece to the figure from the current point
//
//------------------------------------------------------------------------------
HRESULT
CFigureData::LinesTo(
    __in_ecount(count) const MilPoint2F *rgPoints,
        // In: Array of points to add
    INT count)
        // In: Number of points
{
    // Nothing should be added to a closed figure
    HRESULT hr = S_OK;

    if (!rgPoints  ||  count <= 0)
    {
        IFC(E_INVALIDARG);
    }

    if (IsEmpty()  ||  IsClosed())
    {
        IFC(E_UNEXPECTED);
    }
         
    // Add the lines
    IFC(AddSegments(count, count, rgPoints, MilCoreSeg::TypeLine));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::BezierTo
//
//  Synopsis:
//      Add a single Bezier segment to the figure
//
//------------------------------------------------------------------------------
HRESULT
CFigureData::BezierTo(
        IN REAL x1, REAL y1, 
            // Second Bezier point
        IN REAL x2, REAL y2,
            // Third Bezier point
        IN REAL x3, REAL y3,
            // Fourth Bezier point
        bool fSmoothJoin) 
            // = true if forcing a smooth join
{
    MilPoint2F pt[3];

    DUMP_XY(L"Bezier 1 ", x1, y1);
    DUMP_XY(L"Bezier 2 ", x2, y2);
    DUMP_XY(L"Bezier 3 ", x3, y3);

    // Nothing should be added to a closed figure
    HRESULT hr = E_UNEXPECTED;
    if (IsEmpty()  ||  IsClosed())
    {
        goto Cleanup;
    }
       
    // Add the Bezier segment
    pt[0].X = x1;
    pt[0].Y = y1;
    pt[1].X = x2;
    pt[1].Y = y2;
    pt[2].X = x3;
    pt[2].Y = y3;

    IFC(AddSegments(3, 1, pt, MilCoreSeg::TypeBezier, fSmoothJoin));
    
    SetCurved();

Cleanup:
    return hr;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::BeziersTo
//
//  Synopsis:
//      Add a composite Bezier to the figure from the current point
//
//------------------------------------------------------------------------------
HRESULT
CFigureData::BeziersTo(
    __in_ecount(count) const MilPoint2F *rgPoints,
        // Array of points
    IN INT count)
        // Number of points
{
    HRESULT hr = S_OK;

    if (!rgPoints  ||  count < 3)
    {
        IFC(E_INVALIDARG);
    }

    // Nothing should be added to a closed figure
    if (IsEmpty()  ||  IsClosed())
    {
        IFC(E_UNEXPECTED);
    }
       
    // Add the curve
    int cSegments = count / 3;

    if (cSegments * 3 == count)
    {
        IFC(AddSegments(count, cSegments, rgPoints, MilCoreSeg::TypeBezier));
    }
    else
    {
        IFC(E_INVALIDARG);
    }
    
    SetCurved();


Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigure::ArcTo
//
//  Synopsis:
//      Add an elliptical arc to the figure
//
//  Notes:
//      This method implements the SVG elliptical arc spec. The ellipse from
//      which the arc is carved is axis-aligned in its own coordinates, and
//      defined there by its x and y radii. The rotation angle defines how the
//      ellipse's axes are rotated relative to our x axis. The start and end
//      points define one of 4 possible arcs; the sweep and large-arc flags
//      determine which one of these arcs will be chosen. See SVG spec for
//      details.
//
//------------------------------------------------------------------------------
HRESULT
CFigureData::ArcTo(
    IN FLOAT xRadius,    // The ellipse's X radius
    IN FLOAT yRadius,    // The ellipse's Y radius
    IN FLOAT rRotation,  // Rotation angle of the ellipse's x axis
    IN BOOL fLargeArc,   // Choose the larger of the 2 possible arcs if TRUE
    IN BOOL fSweepUp,    // Sweep the arc while increasing the angle if TRUE
    IN FLOAT xEnd,       // X coordinate of the last point
    IN FLOAT yEnd)       // Y coordinate of the last point
{    
    HRESULT hr = S_OK;
    MilPoint2F pt[12];
    int cPieces;

    if (xRadius < 0  || yRadius < 0)
    {
        IFC(E_INVALIDARG);
    }

    // The figure should be started but not closed
    if (IsEmpty()  ||  IsClosed())
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // Get the approximation of the arc with Bezier curves
    ArcToBezier(GetEndPoint().X,
                GetEndPoint().Y,
                xRadius, 
                yRadius, 
                rRotation, 
                fLargeArc, 
                fSweepUp, 
                xEnd, 
                yEnd, 
                pt, 
                cPieces);

    if (0 == cPieces)
    {
        // We have a zero radius, add a straight line segment instead of an arc
        IFC(LineTo(xEnd, yEnd));
    }
    else if (cPieces > 0)
    {
        if (cPieces > 1)
        {
            // Enforcing smoothness at the joins between the pieces 
            IFC(AddSegments(3 * (cPieces - 1), cPieces - 1, &pt[3], MilCoreSeg::TypeBezier, true));
        }

        // The join at the end is not necessarily smooth 
        IFC(AddSegments(3, 1, pt, MilCoreSeg::TypeBezier, false));
    }
    
    // Wrap up
    SetCurved();
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      ClampRoundedRectangleRadius
//
//  Synopsis:
//      Get a signed radius that will be used for construction purposes
//
//  Note:
//      We want to Clamp the radius to fit within the corresponding dimension
//
//------------------------------------------------------------------------------
void
ClampRoundedRectangleRadius(
    REAL rDimension,                // The dimension, width or height
    __inout_ecount(1) REAL &rRadius)// The corresponding radius, clamped here
{
    REAL r = rDimension / 2;

    Assert (_isnan(rRadius) || rRadius >= 0.0);
    Assert (_isnan(rDimension) || rDimension >= 0.0);
    
    // Clamp the radius
    if (rRadius > r)
    {
        rRadius = r;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::InitBufferWithRectanglePoints
//
//  Synopsis:
//      Fill in the buffer with points that make up the rectangle.
//
//------------------------------------------------------------------------------
void
CFigureData::InitBufferWithRectanglePoints(
    __out_ecount_full(4) MilPoint2F *pts,
        // The point buffer
    __in_ecount(1) const MilRectF &rect)
        // The rectangle
{
    pts[0].X = rect.left;
    pts[0].Y = rect.top;
    pts[1].X = rect.right;
    pts[1].Y = rect.top;
    pts[2].X = rect.right;
    pts[2].Y = rect.bottom;
    pts[3].X = rect.left;
    pts[3].Y = rect.bottom;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::InitBufferWithRectanglePoints
//
//  Synopsis:
//      Fill in the buffer with points that make up the rectangle.
//
//------------------------------------------------------------------------------
void
CFigureData::InitBufferWithRoundedRectanglePoints(
    __out_ecount_full(16) MilPoint2F *pts,
        // The point buffer
    __in_ecount(1) const CMilRectF &rect,
        // The rectangle
    REAL rRadiusX,
        // The X-radius of the corner (elliptical arc)
    REAL rRadiusY
        // The Y-radius of the corner (elliptical arc)
    )
{
    ClampRoundedRectangleRadius(rect.Width(), rRadiusX);
    ClampRoundedRectangleRadius(rect.Height(), rRadiusY);

    // Note "1 - ARC_AS_BEZIER" - because we measure from the edge of the rectangle,
    // not the center of the arc.

    REAL rBezierX = REAL((1.0 - ARC_AS_BEZIER) * rRadiusX);
    REAL rBezierY = REAL((1.0 - ARC_AS_BEZIER) * rRadiusY);

    pts[1].X  = pts[0].X  = pts[15].X = pts[14].X = rect.left;
    pts[2].X  = pts[13].X = rect.left  + rBezierX;
    pts[3].X  = pts[12].X = rect.left  + rRadiusX;
    pts[4].X  = pts[11].X = rect.right  - rRadiusX;
    pts[5].X  = pts[10].X = rect.right  - rBezierX;
    pts[6].X  = pts[7].X  = pts[8].X  = pts[9].X  = rect.right;

    pts[2].Y  = pts[3].Y  = pts[4].Y  = pts[5].Y  = rect.top;
    pts[1].Y  = pts[6].Y  = rect.top  + rBezierY;
    pts[0].Y  = pts[7].Y  = rect.top  + rRadiusY;
    pts[15].Y = pts[8].Y  = rect.bottom - rRadiusY;
    pts[14].Y = pts[9].Y  = rect.bottom - rBezierY;
    pts[13].Y = pts[12].Y = pts[11].Y = pts[10].Y = rect.bottom;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::InitAsRectangle
//
//  Synopsis:
//      Set a new figure as a rectangle
//
//------------------------------------------------------------------------------
HRESULT
CFigureData::InitAsRectangle(
    __in_ecount(1) const CMilRectF &rect)  // The rectangle
{
    HRESULT hr = S_OK;
    
    Assert(IsEmpty()); // Should only be called on an empty stomach !!
    
    // Set the vertices
    MilPoint2F pt[5];

    InitBufferWithRectanglePoints(pt, rect);
    pt[4] = pt[0];
    
    IFC(AddSegments(5, 4, pt, MilCoreSeg::TypeLine));
    SetClosed();
    SetAsRectangle();

Cleanup:    
    if (FAILED(hr))
    {
        Reset();
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::InitAsEllipse
//
//  Synopsis:
//      Sets the figure to an ellipse, using the given defining rectangle.
//
//  Note:
//      The TryTessellateFillEllipse method depends on the exact representation
//      constructed here, including the order of the defining points.  So if you
//      change the representation you need to update TryTessellateFillEllipse
//      accordingly.
//

HRESULT CFigureData::InitAsEllipse(
    REAL rCenterX, REAL rCenterY,   // The center of the ellipse
    REAL rRadiusX,                  // The X-radius of the ellipse
    REAL rRadiusY                   // The Y-radius of the ellipse
    )
{
    Assert(IsEmpty()); // Should only be called on an empty stomach !!
    
    HRESULT hr = S_OK;

    MilPoint2F *pPoints = NULL;

    IFC(AddPoints(13, pPoints));
    IFC(AddAndSetTypes(4, MilCoreSeg::TypeBezier, true));

    {   
        REAL rMid;
        
        // Set the X coordinates
        rMid = FLOAT(rRadiusX * ARC_AS_BEZIER);
        
        pPoints[0].X = pPoints[1].X = pPoints[11].X = pPoints[12].X = rCenterX + rRadiusX;
        pPoints[2].X = pPoints[10].X = rCenterX + rMid; 
        pPoints[3].X = pPoints[9].X = rCenterX; 
        pPoints[4].X = pPoints[8].X = rCenterX - rMid; 
        pPoints[5].X = pPoints[6].X = pPoints[7].X  = rCenterX - rRadiusX;
        
        // Set the Y coordinates
        rMid = FLOAT(rRadiusY * ARC_AS_BEZIER);
        
        pPoints[2].Y = pPoints[3].Y = pPoints[4].Y = rCenterY + rRadiusY;
        pPoints[1].Y = pPoints[5].Y = rCenterY + rMid; 
        pPoints[0].Y = pPoints[6].Y = pPoints[12].Y = rCenterY; 
        pPoints[7].Y = pPoints[11].Y = rCenterY - rMid; 
        pPoints[8].Y = pPoints[9].Y = pPoints[10].Y = rCenterY - rRadiusY;
        
        SetCurved();       
        SetClosed();
    }
    
Cleanup:
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::InitAsRoundedRectangle
//
//  Synopsis:
//      Sets the figure to a rectangle with rounded corners.
//
//------------------------------------------------------------------------------
HRESULT 
CFigureData::InitAsRoundedRectangle(
    __in_ecount(1) const CMilRectF &rect,
        // The bounding rectangle
    IN REAL rRadiusX,
        // The X-radius of the corner (elliptical arc)
    IN REAL rRadiusY
        // The Y-radius of the corner (elliptical arc)
    )
{
    Assert(IsEmpty()); // Should only be called on an empty stomach !!
    
    HRESULT hr = S_OK;

    MilPoint2F *pPoints = NULL;
    BYTE *pTypes = NULL;
    
    BYTE bTypeLine = MilCoreSeg::TypeLine | MilCoreSeg::SmoothJoin;
    BYTE bTypeBezier = MilCoreSeg::TypeBezier | MilCoreSeg::SmoothJoin;
    
    IFC(m_rgPoints.ReserveSpace(17, true /* exactly 17 */));
    IFC(AddPoints(17, pPoints));
    
    if (0 != (FigureFlagGapState & m_bFlags))
    {
        bTypeLine |= MilCoreSeg::IsAGap;
        bTypeBezier |= MilCoreSeg::IsAGap;
    }

    IFC(AddTypes(8, pTypes));

    // We start with the top-left arc, and proceed clockwise. The last segment is the line
    // on the left edge.
    // WARNING: The special case tessellation relies on this order. If you change it then
    // you must change CRoundedRectTessellator accordingly

    {
        // Init points
        InitBufferWithRoundedRectanglePoints(pPoints, rect, rRadiusX, rRadiusY);
        pPoints[16] = pPoints[0];

        // Init types

        pTypes[0] = pTypes[2] = pTypes[4] = pTypes[6] = bTypeBezier;
        pTypes[1] = pTypes[3] = pTypes[5] = pTypes[7] = bTypeLine;

        // Init flags

        SetCurved();
        SetClosed();
    }

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::InitAsBeveledRectangle
//
//  Synopsis:
//      Sets the figure to a rectangle with beveled corners.
//
//      Here, the bevel offset is defined to be the distance from the tip of the
//      rectangle to where the bevel starts:
//
//          <- offset ->
//
//          |----------/--
//          |        . 
//          |      .
//          |     .
//          |   .
//          |  .
//          |.
//          /
//          |
//
//------------------------------------------------------------------------------
HRESULT 
CFigureData::InitAsBeveledRectangle(
    __in_ecount(1) const CMilRectF &rect,
        // The bounding rectangle
    IN REAL rBevelOffset
        // The bevel offset
    )
{
    Assert(IsEmpty()); // Should only be called on an empty stomach !!
    
    HRESULT hr = S_OK;

    MilPoint2F *pPoints = NULL;
    BYTE *pTypes = NULL;
    
    BYTE bTypeLine = MilCoreSeg::TypeLine;
    
    IFC(m_rgPoints.ReserveSpace(9, true /* exactly 9 */));
    IFC(AddPoints(9, pPoints));
    
    if (0 != (FigureFlagGapState & m_bFlags))
    {
        bTypeLine |= MilCoreSeg::IsAGap;
    }

    IFC(AddTypes(8, pTypes));

    //
    // We start with the top-left bevel and proceed clockwise. The last segment is the line
    // on the left edge.
    //

    {
        // Init points

        //
        // BevelOffset can be no greater than 0.5 times the minimum
        // dimensions of the rectangle.
        // 
        rBevelOffset = ClampReal(
            rBevelOffset,
            0.0f /* min */,
            0.5f * min(rect.Width(), rect.Height()) /* max */);
                            
        pPoints[0].X = pPoints[7].X = rect.left;
        pPoints[1].X = pPoints[6].X = rect.left + rBevelOffset;
        pPoints[2].X = pPoints[5].X = rect.right - rBevelOffset;
        pPoints[3].X = pPoints[4].X = rect.right;

        pPoints[1].Y = pPoints[2].Y = rect.top;
        pPoints[0].Y = pPoints[3].Y = rect.top + rBevelOffset;
        pPoints[7].Y = pPoints[4].Y = rect.bottom - rBevelOffset;
        pPoints[6].Y = pPoints[5].Y = rect.bottom;

        pPoints[8] = pPoints[0];

        // Init types

        pTypes[0] = pTypes[1] = pTypes[2] = pTypes[3] = bTypeLine;
        pTypes[4] = pTypes[5] = pTypes[6] = pTypes[7] = bTypeLine;

        // Init flags

        SetClosed();
    }

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureData::InitFromRawData
//
//  Synopsis:
//      Initialize from arrays of points and segment-types + transformation.
//
//  Notes:
//      This is a low level utility that trusts the caller.  The validity of the
//      data is asserted but not checked.  The first and last points are assumed
//      to be eaqual if pTypes[0] & MilCoreSeg::Closed.
//
//------------------------------------------------------------------------------
HRESULT
CFigureData::InitFromRawData(
    __in UINT cPoints,
        // Point count
    __in UINT cSegments,
        // Segment count
    __in_ecount(cPoints) MilPoint2D *pPoints,
        // Points
    __in_ecount(cSegments) byte *pTypes,
        // Types
    __in_ecount_opt(1) CMILMatrix *pMatrix)
        // A transformation to apply to the points
{
    HRESULT hr = S_OK;
    UINT u;
    MilPoint2F ptF;

#if DBG
    // Validate the data.  
    UINT check = 0;
    for (u = 0;  u < cSegments;  u++)
    {
        if ((pTypes[u] & static_cast<BYTE>(MilCoreSeg::TypeMask)) == MilCoreSeg::TypeLine)
        {
            check++;
        }
        else
        {
            Assert((pTypes[u] & static_cast<BYTE>(MilCoreSeg::TypeMask)) == MilCoreSeg::TypeBezier);
            check += 3;
        }
    }

    Assert((cSegments > 0  &&  check + 1 == cPoints)  ||  // + 1 for the start point
           (cPoints == 0  ||  cPoints == 1));
#endif  // DBG

    // This loop will be replaced by AddMultiple if and when m_rgPoints switches to doubles.
    for (u = 0;  u < cPoints;  u++)
    {
        ptF.X = static_cast<float>(pPoints[u].X);
        ptF.Y = static_cast<float>(pPoints[u].Y);

        if (pMatrix)
        {
            pMatrix->Transform(&ptF, &ptF);
        }

        IFC(m_rgPoints.Add(ptF));
    }

    // Strip the Closed bit when copying the types
    IFC(m_rgTypes.AddAndSet(1, pTypes[0] & (~MilCoreSeg::Closed)));
    IFC(m_rgTypes.AddMultipleAndSet(pTypes + 1, cSegments - 1));

    if (pTypes[0] & MilCoreSeg::Closed)
    {
        // It's a closed figure.  SetClosed will assert if first != last point.
        SetClosed();
    }
Cleanup:
    RRETURN(hr);
}
//////////////////////////////////////////////////////////////////////////////
//
//                       Implementation of CFigure
//

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigure::Clone
//
//  Synopsis:
//      Clone a new figure from this
//
//------------------------------------------------------------------------------
HRESULT
CFigure::Clone(
    __deref_out_ecount(1) CFigure *& pClone) const
        // Out: The clone
{
    HRESULT hr = E_OUTOFMEMORY;
    CFigure *pCopy = NULL;

    pClone = NULL;
    
    IFCOOM(pCopy = new CFigure);
    IFC(pCopy->m_oData.SetFrom(m_oData));

    // Cloning successful
    pClone = pCopy;

Cleanup:
    if (!pClone)
    {
        delete pCopy;
    }

    RRETURN(hr);
}




