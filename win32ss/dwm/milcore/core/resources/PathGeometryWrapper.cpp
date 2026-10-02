// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//      $File name: PathGeometryWrapper.cpp
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

// Description:
//
// PathGeometryData implements IShapeData, an interface that abstracts away direct 
// knowledge of the geometry storage representation.  This wrapper class understands a
// linear shape data representation constructed by the managed Geometry classes.  This
// is necessary to bridge the gap between our managed Geometry classes and low level
// computational geometry services.
//
// First, milcoretypes.w contains the definition of some structs, each associated with
// a Geometry related class (some properties for background are in braces)
//
// MilPathGeometry - PathGeometry  (eg, fill rule, figure count, etc.)
// MilPathFigure - PathFigure  (eg,  IsFillable, segment count, etc.)
// MilSegmentLine - LineSegment (eg, Point)
// MilSegmentBezier - BezierSegment (eg, Point1, Point2, Point3)
// MilSegmentQuadraticBezier - QuadraticBezierSegment (eg, Point1, Point2)
// MilSegmentArc - ArcSegment (eg, Point, Size, XRotation, etc)
// MilSegmentPoly - PolyLineSegment, PolyBezierSegment, PolyQuadraticBezierSegment
//                    (eg, Count)
//
// This class is passed a chunk of memory formatted as follows:
//
// (A)        (B)                                                  (C)
//  +----------+----------+----------+---+---------+----------+---
//  | MIL_PATH | MIL_PATH | MIL_SEG_ |   | MIL_PATH | MIL_SEG |
//  | GEOMETRY | FIGURE   |   LINE   |...|  FIGURE  |   LINE  |...
//  |          |          |          |   |          |         |
//  +----------+----------+----------+---+----------+---------+---
//
// 1. The header always begins with a MilPathGeometry struct.
// 2. This is followed by a MilPathFigure struct
// 3. This is followed by MIL_SEGMENT_* structs (the number of such segments is in
//    MilPathFigure)
// 4. The pattern of (2) and (3) is repeated for the number of figures 
//    (in MIL_PATHGEOMETY)
// 
// The only twist on the above is the case of MilSegmentPoly struct, which represents
// a set of points interpreted as poly line, poly bezier, or poly quadratic bezier 
// points.  These points immediately follow the struct.  The size of computed by taking 
// MilSegmentPoly.Count*sizeof(MilPoint2D).
//
// In this way, the PathGeometry object tree consisting of PathFigures, 
// PathFigureCollections, PathSegmentCollections, PathSegments, etc. is flattened into
// an easily accessible linear representation.  All content needed to resolve 
// animations and compute instantaneous values are contained within.
//
// PathGeometryData understands how to address this structure.  It mains a "current 
// figure" state that the caller can use to traverse the figures and access properties,
// etc. 
//
// PathFigureData implements IFigureData, an interface that abstracts away direct 
// knowledge of the figure storage representation.  This wrapper class understands how
// to deal with memory formatted as between (B) and (C) above.  It provides services to
// query per figure properties, and enumerate the segments of the figure.  It mains a 
// "current segment" state the caller can use to traverse the segments and access 
// properties, etc.
//
// See mil\geometry\shape.* and mil\geometry\figure.* for details on the usage pattern
// of IShapeData and IFigureData.
//
// NOTE: To ensure we do not hit misalignment exceptions in 64-bit, all structs are
// packed to be 8-byte aligned.

#include "precomp.hpp"

inline MilPoint2F ConvertToSingle(MilPoint2D pt)
{
    MilPoint2F ptF;
    ptF.X = static_cast<FLOAT> (pt.X);
    ptF.Y = static_cast<FLOAT> (pt.Y);
    return ptF;
}


//+-----------------------------------------------------------------------------
//
//  Class:
//      PathFigureData
//
//  Synopsis:
//      Interface for access and queries on figure data
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::PathFigureData
//
//  Synopsis:
//      Constructor for PathFigureData
//
//------------------------------------------------------------------------------
PathFigureData::PathFigureData()
{
    SetFigureData(NULL, 0, NULL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::PathFigureData
//
//  Synopsis:
//      Initialize contents of PathFigureData through constructor
//
//------------------------------------------------------------------------------
PathFigureData::PathFigureData(MilPathFigure *pFigure, UINT nSize, const CMILMatrix *pMatrix)
{
    SetFigureData(pFigure, nSize, pMatrix);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::SetFigureData
//
//  Synopsis:
//      Initialize contents of PathFigureData
//
//------------------------------------------------------------------------------
VOID PathFigureData::SetFigureData(MilPathFigure *pFigure, UINT nSize, const CMILMatrix *pMatrix)
{
    m_pFigure = pFigure;
    m_nSize = nSize;

    if (pMatrix && pMatrix->IsIdentity())
    {
        m_pMatrix = NULL;
    }
    else
    {
        m_pMatrix = pMatrix;
    }

    m_pCurSegment = GetFirstSegment();
    Assert(m_pCurSegment != NULL);

    m_uCurIndex = m_uStop = m_uInnerStop = UINT_MAX;
    m_uInnerIndex = 0;
    
    m_fEndPointValid = FALSE;

    m_uCurPoint = 0;

}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::SetInnerIndexToLast
//
//  Synopsis:
//      Set the inner segment index to the last inner index
//
//------------------------------------------------------------------------------
void PathFigureData::SetInnerIndexToLast() const
{
    Assert(m_pCurSegment != NULL);
    
    switch (m_pCurSegment->Type)
    {
        case MilSegmentType::None: 
        case MilSegmentType::Line: 
        case MilSegmentType::Bezier: 
        case MilSegmentType::QuadraticBezier: 
            m_uInnerIndex = 0;
            break;
            
        case MilSegmentType::Arc:
            // Arc data should have been set in SetArcData, which should have
            // been called before this call
            m_uInnerIndex = m_uLastInnerIndex;
            m_uCurPoint = 3 * m_uLastInnerIndex;
            break;
            
        case MilSegmentType::PolyLine: 
            {
                const MilSegmentPoly *pPolySegment = static_cast<MilSegmentPoly*> (m_pCurSegment);
                Assert(pPolySegment->Count != 0);

                m_uInnerIndex = pPolySegment->Count - 1;
                break;
            }
            
        case MilSegmentType::PolyBezier: 
            {
                const MilSegmentPoly *pPolySegment = static_cast<MilSegmentPoly*> (m_pCurSegment);
                Assert(pPolySegment->Count % 3 == 0);
                Assert(pPolySegment->Count >=3);

                m_uInnerIndex = pPolySegment->Count / 3 - 1;
                break;
            }
            
      case MilSegmentType::PolyQuadraticBezier: 
            {
                const MilSegmentPoly *pPolySegment = static_cast<MilSegmentPoly*> (m_pCurSegment);
                Assert((pPolySegment->Count & 1) == 0);
                Assert(pPolySegment->Count >= 2);
                
                m_uInnerIndex = (pPolySegment->Count / 2) - 1;
                break;
            }
            
        default: 
            Assert(FALSE); 
            break;
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::IsEmpty
//
//  Synopsis:
//      Returns true if figure is empty.
//
//------------------------------------------------------------------------------
bool PathFigureData::IsEmpty() const
{
    // The figure is never empty because it always have at least a start point
    return false;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::HasNoSegments
//
//  Synopsis:
//      Returns true if there are no segments beyond the start.
//
//------------------------------------------------------------------------------
bool PathFigureData::HasNoSegments() const
{
    Assert(m_pFigure != NULL);

    return m_pFigure->Count == 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::IsClosed
//
//  Synopsis:
//      Returns if figure is closed.
//
//------------------------------------------------------------------------------
bool PathFigureData::IsClosed() const
{
    Assert(m_pFigure != NULL);
    return (m_pFigure->Flags & MilPathFigureFlags::IsClosed) != 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::IsAtASmoothJoin
//
//  Synopsis:
//      Returns true if the join at the end of the current inner segment ought
//      to be accepted as smooth without checking.
//
//------------------------------------------------------------------------------
bool PathFigureData::IsAtASmoothJoin() const
{
    Assert(m_pFigure != NULL);
    
    Assert(m_pCurSegment != NULL);

    return (((m_pCurSegment->Flags & MilCoreSeg::SmoothJoin)  != 0)     || 
           (m_pCurSegment->Type == MilSegmentType::Arc  &&  m_uInnerIndex < m_uLastInnerIndex));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::HasGaps
//
//  Synopsis:
//      Returns true if figure has gaps.
//
//------------------------------------------------------------------------------
bool PathFigureData::HasGaps() const
{
    Assert(m_pFigure != NULL);
    return (m_pFigure->Flags & MilPathFigureFlags::HasGaps) != 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::IsAtAGap
//
//  Synopsis:
//      Returns true if the current segment is a gap (not to be stroked).
//
//------------------------------------------------------------------------------
bool PathFigureData::IsAtAGap() const
{  
    bool fIsAtGap = false;

    if (m_uCurIndex >= m_pFigure->Count)
    {
        // We're at the implied closing line segment
        fIsAtGap = false;
    }
    else
    {
        Assert(m_pCurSegment != NULL);

        fIsAtGap = ((m_pCurSegment->Flags & MilCoreSeg::IsAGap) != 0);
    }

    return fIsAtGap;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::IsFillable
//
//  Synopsis:
//      Returns true if figure is fillable.
//
//------------------------------------------------------------------------------
bool PathFigureData::IsFillable() const
{
    Assert(m_pFigure != NULL);
    return (m_pFigure->Flags & MilPathFigureFlags::IsFillable) != 0;
}    

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::IsAxisAlignedRectangle
//
//  Synopsis:
//      Return true if this figure is an axis aligned rectangle
//
//------------------------------------------------------------------------------
bool PathFigureData::IsAxisAlignedRectangle() const
{
    bool fIsAxisAlignedRect = false;

    if (IsAParallelogram())
    {
        // We can get four points that might be a rectangle
        MilPoint2F points[4];
        GetParallelogramVertices(points);

        fIsAxisAlignedRect = (RectF_RBFromParallelogramPointsF(points, NULL) == TRUE);
    }

    return fIsAxisAlignedRect;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::GetAsRectangle
//
//  Synopsis:
//      Gets the rectangle of an rectangle figure.
//
//  Notes:
//      The rect returned may not be well ordered.
//
//------------------------------------------------------------------------------
VOID PathFigureData::GetAsRectangle(__out_ecount(1) MilRectF &rect) const
{
    Assert(IsAxisAlignedRectangle());

    MilPoint2F points[4];
    GetParallelogramVertices(points);

    rect.left = points[0].X;
    rect.top = points[0].Y;

    rect.right = points[2].X;
    rect.bottom = points[2].Y;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::GetAsWellOrderedRectangle
//
//  Synopsis:
//      Gets the rectangle of an rectangle figure. This rectangle is guaranteed
//      to have positive width and height.
//
//------------------------------------------------------------------------------
VOID PathFigureData::GetAsWellOrderedRectangle(__out_ecount(1) MilRectF &rect) const
{
    Assert(IsAxisAlignedRectangle());

    MilPoint2F points[4];
    GetParallelogramVertices(points);

    RectF_RBFromParallelogramPointsF(points, &rect);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::GetParallelogramVertices
//
//  Synopsis:
//      Get the 4 vertices of this parallelogram figure
//
//  Synopsis:
//      The caller is responsible for calling it only on a figure that has been
//      constructed as a (possibly transformed) rectangle with a single 3-point
//      polyline.  This assumption is guarded by assertions.
//
//------------------------------------------------------------------------------
VOID
PathFigureData::GetParallelogramVertices(
    __out_ecount(4) MilPoint2F *pVertices,
        // 4 Rectangle's vertices
    __in_ecount_opt(1) const CMILMatrix *pMatrix) const
        // Transformation (optional)
{
    UINT i;
    CMILMatrix combined;

    Assert(m_pFigure->Count == 1);

    MilSegment *pSegment = GetFirstSegment();
    Assert(MilSegmentType::PolyLine == m_pCurSegment->Type);

    MilSegmentPoly *pPoly = static_cast<MilSegmentPoly*>(pSegment);
    Assert(3 == pPoly->Count);

    // First vertex
    pVertices[0] = ConvertToSingle(m_pFigure->StartPoint);

    // The remaining 3 vertices
    const MilPoint2D *pPoints = reinterpret_cast<MilPoint2D*>(pPoly+1);
    for (i = 1;  i <= 3;  i++)
    {
        pVertices[i] = ConvertToSingle(pPoints[i-1]);
    }
    
    if (m_pMatrix)
    {
        if (pMatrix)
        {
            combined = *m_pMatrix;
            combined.Multiply(*pMatrix);
            pMatrix = &combined;
        }
        else
        {
            pMatrix = m_pMatrix;
        }
    }
        
    if (pMatrix)
    {
        // Transform
        for (i = 0;  i <= 3;  i++)
        {
            TransformPoint(*pMatrix, pVertices[i]);
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::GetCountsEstimate
//
//  Synopsis:
//      Computes a conservative estimate of the number of segments and point
//      needed for this figure
//
//  Notes:
//      The estimate may not be tight because an arc segment may generate 1,2,3
//      or 4 segments.
//
//------------------------------------------------------------------------------
HRESULT
PathFigureData::GetCountsEstimate(
    OUT UINT &cSegments,    // An estimate of the nunber of segments 
    OUT UINT &cPoints       // An estimate of the number of points
    ) const
{
    HRESULT hr = S_OK;

    Assert(m_pFigure != NULL);
    UINT uIndex;
    MilSegment *pSegment = GetFirstSegment();

    cPoints = 1;  // for the start point
    cSegments = 0;

    if (!pSegment)
        goto Cleanup;
    
    for (uIndex = 0;  uIndex < m_pFigure->Count;  uIndex++)
    {
        switch (pSegment->Type)
        {
            case MilSegmentType::Line: 
                IFC(IncrementUINT(IN OUT cSegments));
                IFC(IncrementUINT(IN OUT cPoints));

                pSegment = reinterpret_cast<MilSegment*> 
                    (reinterpret_cast<BYTE*>(pSegment) + sizeof(MilSegmentLine));
                break;

            case MilSegmentType::Bezier: 
                IFC(IncrementUINT(IN OUT cSegments));
                IFC(AddUINT(cPoints, 3, OUT cPoints));

                pSegment = reinterpret_cast<MilSegment*> 
                    (reinterpret_cast<BYTE*>(pSegment) + sizeof(MilSegmentBezier));
                break;

            case MilSegmentType::QuadraticBezier:
                IFC(IncrementUINT(IN OUT cSegments));
                IFC(AddUINT(cPoints, 3, OUT cPoints));

                pSegment = reinterpret_cast<MilSegment*> 
                    (reinterpret_cast<BYTE*>(pSegment) + sizeof(MilSegmentQuadraticBezier));
                break;

            case MilSegmentType::Arc:
                // An arc may have up to 4 Bezier segments
                IFC(AddUINT(cSegments, 4, OUT cSegments));
                IFC(AddUINT(cPoints, 12, OUT cPoints));

                pSegment = reinterpret_cast<MilSegment*> 
                    (reinterpret_cast<BYTE*>(pSegment) + sizeof(MilSegmentArc));
                break;
            
            case MilSegmentType::PolyLine:
                {
                    const MilSegmentPoly *pPoly = static_cast<MilSegmentPoly*> (pSegment);
                    IFC(AddUINT(cSegments, pPoly->Count, OUT cSegments));
                    IFC(AddUINT(cPoints, pPoly->Count, OUT cPoints));

                    pSegment = reinterpret_cast<MilSegment*> 
                        (reinterpret_cast<BYTE*>(pSegment) 
                        + sizeof(MilSegmentPoly)
                        + sizeof(MilPoint2D)*pPoly->Count);
                    break;
                }

            case MilSegmentType::PolyBezier:
                {
                    const MilSegmentPoly *pPoly = static_cast<MilSegmentPoly*> (pSegment);
                    IFC(AddUINT(cSegments, pPoly->Count / 3, OUT cSegments));
                    IFC(AddUINT(cPoints, pPoly->Count, OUT cPoints));

                    pSegment = reinterpret_cast<MilSegment*> 
                        (reinterpret_cast<BYTE*>(pSegment) 
                        + sizeof(MilSegmentPoly) 
                        + sizeof(MilPoint2D)*pPoly->Count);
                    break;
                }

            case MilSegmentType::PolyQuadraticBezier:
                {
                    const MilSegmentPoly *pPoly = static_cast<MilSegmentPoly*> (pSegment);
                    UINT cInnerSegments = pPoly->Count / 2;
                    IFC(AddUINT(cSegments, cInnerSegments, OUT cSegments));

                    // Quadratic segments will be converted to cubics, requiring 3 points each 
                    IFC(MultiplyUINT(cInnerSegments, 3, OUT cInnerSegments));
                    IFC(AddUINT(cPoints, cInnerSegments, OUT cPoints));

                    pSegment = reinterpret_cast<MilSegment*> 
                        (reinterpret_cast<BYTE*>(pSegment)
                        + sizeof(MilSegmentPoly)
                        + sizeof(MilPoint2D)*pPoly->Count);
                    break;
                }
            default:
                {
                    RIP("Invalid segment type.");
                    break;
                }
        }

        if (IsClosed())
        {
            IFC(IncrementUINT(IN OUT cSegments));
            IFC(IncrementUINT(IN OUT cPoints));
        }

    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::GetCurrentSegment
//
//  Synopsis:
//      Returns the type and points of the current segment.
//
//  Returns:
//      true if this is the segment where a stop has been set.
//
//  Notes:
//      The type is either a line or bezier.  The points are either a single
//      point (for a line) or three points (for a cubic bezier.)
//
//------------------------------------------------------------------------------
bool PathFigureData::GetCurrentSegment(
    __out_ecount(1) BYTE            &bType,
        // Segment type (line or Bezier)
    __deref_outro_xcount((bType == MilCoreSeg::TypeLine) ? 1 : 3) const MilPoint2F *&pt
        // Line endpoint or curve 3 last points
    ) const
{
    Assert(m_pFigure != NULL);
    Assert(m_pCurSegment != NULL);
            
    MilPoint2F *pptPoints = static_cast<MilPoint2F*> (&m_rgPoints[0]);


    if (m_uCurIndex >= m_pFigure->Count)
    {
        // We're at the implied closing line segment
        bType = MilCoreSeg::TypeLine;
        pptPoints[0] = GetStartPoint();
        goto Cleanup;
    }

    switch (m_pCurSegment->Type)
    {
        case MilSegmentType::Line: 
            {
                const MilSegmentLine *pLine = static_cast<MilSegmentLine*> (m_pCurSegment);
                bType = MilCoreSeg::TypeLine;

                pptPoints[0] = ConvertToSingle(pLine->Point);
                
                if (m_pMatrix != NULL)
                {
                    TransformPoint(*m_pMatrix, pptPoints[0]);
                }
                break;
            }
        
        case MilSegmentType::Bezier: 
            {
                const MilSegmentBezier *pBezier = static_cast<MilSegmentBezier*> (m_pCurSegment);
                bType = MilCoreSeg::TypeBezier;

                pptPoints[0] = ConvertToSingle(pBezier->Point1);
                pptPoints[1] = ConvertToSingle(pBezier->Point2);
                pptPoints[2] = ConvertToSingle(pBezier->Point3);
                
                if (m_pMatrix != NULL)
                {
                    TransformPoints(*m_pMatrix, 3, &pptPoints[0]);
                }
                break;
            }

        case MilSegmentType::QuadraticBezier: 
            {
                const MilSegmentQuadraticBezier *pBezier = 
                    static_cast<MilSegmentQuadraticBezier*> (m_pCurSegment);
                bType = MilCoreSeg::TypeBezier;
                
                SetQuadraticBezier(
                    *GetCurrentSegmentStartD(), 
                    pBezier->Point1, 
                    pBezier->Point2);
                
                if (m_pMatrix != NULL)
                {
                    TransformPoints(*m_pMatrix, 3, &pptPoints[0]);
                }
                break;
            }

        case MilSegmentType::Arc:
           bType = m_bType;
           pptPoints += m_uCurPoint;
           break;
        
        case MilSegmentType::PolyLine:
            {
                MilSegmentPoly *pPoly = static_cast<MilSegmentPoly*> (m_pCurSegment);
                const MilPoint2D *pPoints = reinterpret_cast<MilPoint2D*>(pPoly+1);
                bType = MilCoreSeg::TypeLine;
                
                pptPoints[0] = ConvertToSingle(pPoints[m_uInnerIndex]);

                if (m_pMatrix != NULL)
                {
                    TransformPoint(*m_pMatrix, pptPoints[0]);
                }
                break;
            }

        case MilSegmentType::PolyBezier:
            {
                MilSegmentPoly *pPoly = static_cast<MilSegmentPoly*> (m_pCurSegment);
                MilPoint2D *pPoints = reinterpret_cast<MilPoint2D*>(pPoly+1);
                UINT uInnerIndexTimesThree = 0;

                // Operation guaranteed to succeed by the marshaling code
                Verify(SUCCEEDED(UIntMult(m_uInnerIndex, 3, &uInnerIndexTimesThree)));

                pPoints = &pPoints[uInnerIndexTimesThree];
                bType = MilCoreSeg::TypeBezier;
                
                pptPoints[0] = ConvertToSingle(*(pPoints++));
                pptPoints[1] = ConvertToSingle(*(pPoints++));
                pptPoints[2] = ConvertToSingle(*pPoints);
                
                if (m_pMatrix != NULL)
                {
                    TransformPoints(*m_pMatrix, 3, &pptPoints[0]);
                }
                break;
            }

        case MilSegmentType::PolyQuadraticBezier:
            {
                MilSegmentPoly *pPoly = static_cast<MilSegmentPoly*> (m_pCurSegment);
                MilPoint2D *pPoints = reinterpret_cast<MilPoint2D*>(pPoly+1);
                UINT uInnerIndexTimesTwo = 0;

                // Operation guaranteed to succeed by the marshaling code
                Verify(SUCCEEDED((UIntMult(m_uInnerIndex, 2, &uInnerIndexTimesTwo))));

                pPoints = &pPoints[uInnerIndexTimesTwo];
                bType = MilCoreSeg::TypeBezier;

                Assert(GetCurrentSegmentStartD());
                
                SetQuadraticBezier(
                    m_uInnerIndex>0 ? *(pPoints-1) : *GetCurrentSegmentStartD(), 
                    *pPoints, 
                    *(pPoints+1));

                if (m_pMatrix != NULL)
                {
                    TransformPoints(*m_pMatrix, 3, &pptPoints[0]);
                }
                break;
            }
                
        default:
            // This case should never be hit, but the output needs to be initialized anyway
            RIP("Invalid segment type");
            bType = MilCoreSeg::TypeLine;
            break;
            
    }
    
Cleanup:
    pt = static_cast<const MilPoint2F *> (pptPoints);
        
    // Returning true if this is the last figure or if this is a stop
    return (m_uCurIndex == m_uStop)  &&  (m_uInnerIndex == m_uInnerStop);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::GetCurrentSegmentStart
//
//  Synopsis:
//      Returns the first point starting the current segment.
//
//------------------------------------------------------------------------------
const MilPoint2F &PathFigureData::GetCurrentSegmentStart() const
{
    Assert(GetCurrentSegmentStartD());

    m_ptStartPoint = ConvertToSingle(*GetCurrentSegmentStartD());

    if (m_pMatrix != NULL)
    {
        TransformPoint(*m_pMatrix, m_ptStartPoint);
    }
   
    return m_ptStartPoint;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::GetStartPoint
//
//  Synopsis:
//      Returns the first point in the figure, transformed
//
//------------------------------------------------------------------------------
const MilPoint2F &PathFigureData::GetStartPoint() const
{
    Assert(m_pFigure != NULL);

    m_ptStartPoint = ConvertToSingle(m_pFigure->StartPoint);

    if (m_pMatrix != NULL)
    {
        TransformPoint(*m_pMatrix, m_ptStartPoint);
    }

    return m_ptStartPoint;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::GetEndPoint
//
//  Synopsis:
//      Returns the last point in the figure.
//
//------------------------------------------------------------------------------
const MilPoint2F &PathFigureData::GetEndPoint() const
{
    Assert(m_pFigure != NULL);

    if (IsClosed())
    {
        return GetStartPoint();
    }

    if (m_fEndPointValid)
    {
        return m_ptEndPoint;
    }

    // We should be at the last segment.
    Assert(GetLastSegment());
    Assert(GetSegmentLastPoint(GetLastSegment()));
    m_ptEndPoint = ConvertToSingle(*GetSegmentLastPoint(GetLastSegment()));

    if (m_pMatrix != NULL)
    {
        TransformPoint(*m_pMatrix, m_ptEndPoint);
    }

    m_fEndPointValid = TRUE;

    return m_ptEndPoint;
}   

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::SetQuadraticBezier
//
//  Synopsis:
//      Set a cubic bezier from a quadratic bezier in the scratch area
//
//------------------------------------------------------------------------------
VOID PathFigureData::SetQuadraticBezier(
    IN MilPoint2D pt0,
    IN MilPoint2D pt1,
    IN MilPoint2D pt2
        // The 3 points defining a quadratic Bezier curve
    ) const
{
    // By the degree-elevation formula for Bezier curves (found in any geometric
    // modeling textbook) the cubic Bezier points of this quadratic Bezier curve  
    // are pt0 (not set here), (1/3)*pt0+ (2/3)*pt1, (2/3)*pt1 + (1/3)*pt2, pt2.

    m_rgPoints[0].X = static_cast<FLOAT> (ONE_THIRD * pt0.X + TWO_THIRDS * pt1.X);
    m_rgPoints[0].Y = static_cast<FLOAT> (ONE_THIRD * pt0.Y + TWO_THIRDS * pt1.Y);

    m_rgPoints[1].X = static_cast<FLOAT> (TWO_THIRDS * pt1.X + ONE_THIRD * pt2.X);
    m_rgPoints[1].Y = static_cast<FLOAT> (TWO_THIRDS * pt1.Y + ONE_THIRD * pt2.Y);

    m_rgPoints[2].X = static_cast<FLOAT> (pt2.X);
    m_rgPoints[2].Y = static_cast<FLOAT> (pt2.Y);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::SetToFirstSegment
//
//  Synopsis:
//      Sets to the first "real" segment.
//
//  Returns:
//      True if set
//
//------------------------------------------------------------------------------
bool PathFigureData::SetToFirstSegment() const
{
    Assert(m_pFigure != NULL);

    bool fSet = (m_pFigure->Count > 0);
    if (fSet)
    {
        m_pCurSegment = GetFirstSegment();
        m_uCurIndex = 0;

        Assert(m_pCurSegment != NULL);

        if (MilSegmentType::Arc == m_pCurSegment->Type)
        {
            SetArcData();
            m_uCurPoint = 0;
        }

        m_uInnerIndex = 0;
    }

    return fSet;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::GetCurrentSegmentStartD
//
//  Synopsis:
//      Helper function that returns the previous segment.
//
//------------------------------------------------------------------------------
__notnull const MilPoint2D *PathFigureData::GetCurrentSegmentStartD() const
{
    if (0 == m_uCurIndex)
    {
        // The first segment starts at the figure's StartPoint
        return &m_pFigure->StartPoint;
    }
    else
    {
        MilSegment *pSegment = m_pCurSegment;
        Assert(pSegment != NULL);

        if (m_uCurIndex < m_pFigure->Count)
        {
            // Get back to the previous segment
            BYTE *p = reinterpret_cast<BYTE*>(pSegment);
            p -= pSegment->BackSize;
            pSegment = reinterpret_cast<MilSegment*>(p);
        }
        //
        // Otherwise we're at the implied closing segment, where the previous segment
        // is m_pCurSegment, to which pSegment has already been set
        //

        return GetSegmentLastPoint(pSegment);
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::GetSegmentLastPoint
//
//  Synopsis:
//      Helper function that returns the last point of the segment.
//
//------------------------------------------------------------------------------
__notnull MilPoint2D *PathFigureData::GetSegmentLastPoint(MilSegment *pSegment) const
{
    Assert(m_pFigure != NULL);
    Assert(pSegment != NULL);
    
    Assert(!IsEmpty());  // Should be checked by the caller

    switch (pSegment->Type)
    {
        case MilSegmentType::Line: 
            {
                MilSegmentLine *pLine = static_cast<MilSegmentLine*>(pSegment);
                return &pLine->Point;
            }
        
        case MilSegmentType::Bezier: 
            {
                MilSegmentBezier *pBezier = static_cast<MilSegmentBezier*>(pSegment);
                return &pBezier->Point3;
            }
        
        case MilSegmentType::QuadraticBezier: 
            {
                MilSegmentQuadraticBezier *pBezier = 
                    static_cast<MilSegmentQuadraticBezier*>(pSegment);
                return &pBezier->Point2;
            }
        
        case MilSegmentType::Arc:
            {
                MilSegmentArc *pArc = static_cast<MilSegmentArc*>(pSegment);
                return &pArc->Point;
            }
            
        case MilSegmentType::PolyLine: 
        case MilSegmentType::PolyBezier: 
        case MilSegmentType::PolyQuadraticBezier: 
            {
                MilSegmentPoly *pPolySegment = static_cast<MilSegmentPoly*> (pSegment);
                Assert(pPolySegment->Count != 0);

                return (reinterpret_cast<MilPoint2D*>(pPolySegment+1)) + (pPolySegment->Count-1);
            }

        default:
            RIP("Invalid segment type.");
    }

    return NULL;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::SetArcData
//
//  Synopsis:
//      Helper function, sets the data needed for consuming an arc segment
//
//------------------------------------------------------------------------------
void PathFigureData::SetArcData() const
{
    const MilSegmentArc *pArc = static_cast<MilSegmentArc*> (m_pCurSegment);
    
    Assert(pArc != NULL);
    Assert(m_pCurSegment->Type == MilSegmentType::Arc);

    INT cPieces, cPoints;

    const MilPoint2D *pLastPoint = GetCurrentSegmentStartD();
    Assert(pLastPoint);
    
    MilPoint2F pt = ConvertToSingle(pArc->Point);
    MilSizeD size = pArc->Size;

    if (IsSizeDotEmpty(&size))
    {
        // This way we will end up drawing nothing.
        size.Width = 0;
        size.Height = 0;
    }

    ArcToBezier(
        static_cast<FLOAT> (pLastPoint->X),
        static_cast<FLOAT> (pLastPoint->Y),
        static_cast<FLOAT> (size.Width),
        static_cast<FLOAT> (size.Height),
        static_cast<FLOAT> (pArc->XRotation),
        pArc->LargeArc,
        pArc->Sweep,
        pt.X,
        pt.Y,
        m_rgPoints,
        cPieces);

    // cPieces = -1 indicates a degenerate line, but we still treat it as a line, so--
    if (cPieces <= 0)      
    {
        // This is a (possibly degenerate) line
        m_rgPoints[0].X = static_cast<FLOAT>(pt.X);
        m_rgPoints[0].Y = static_cast<FLOAT>(pt.Y);
        m_bType = MilCoreSeg::TypeLine;
        cPoints = cPieces = 1;
    }
    else
    {
        m_bType = MilCoreSeg::TypeBezier;
        cPoints = 3 * cPieces;
    }
    
    if (m_pMatrix != NULL)
    {
        TransformPoints(*m_pMatrix, cPoints, m_rgPoints);
    }

    m_uLastInnerIndex = cPieces - 1;
    m_uCurPoint = 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::SetToNextSegment
//
//  Synopsis:
//      Traverse to the next segment in the figure.
//
//  Returns:
//      True if set
//
//------------------------------------------------------------------------------
bool PathFigureData::SetToNextSegment() const
{
    Assert(m_pFigure != NULL);

    bool fSet = false;
    MilSegment *pSegment = NULL; 

    Assert(m_pCurSegment != NULL);

    switch (m_pCurSegment->Type)
    {
        case MilSegmentType::Line: 
            fSet = (m_uCurIndex < m_pFigure->Count - 1);
            if (fSet)
            {
                pSegment = reinterpret_cast<MilSegment*> 
                    (reinterpret_cast<BYTE*>(m_pCurSegment) + sizeof(MilSegmentLine));
            }
            break;

        case MilSegmentType::Bezier: 
            fSet = (m_uCurIndex < m_pFigure->Count - 1);
            if (fSet)
            {
                pSegment = reinterpret_cast<MilSegment*> 
                    (reinterpret_cast<BYTE*>(m_pCurSegment) + sizeof(MilSegmentBezier));
            }
            break;

        case MilSegmentType::QuadraticBezier: 
            fSet = (m_uCurIndex < m_pFigure->Count - 1);
            if (fSet)
            {
                pSegment = reinterpret_cast<MilSegment*> 
                    (reinterpret_cast<BYTE*>(m_pCurSegment) + sizeof(MilSegmentQuadraticBezier));
            }
            break;

        case MilSegmentType::Arc:
            fSet = (m_uInnerIndex < m_uLastInnerIndex);
            if (fSet)
            {
                // Move to the next inner segment
                Assert(MilCoreSeg::TypeBezier == m_bType);  // Otherwise there would be only one segment
                m_uCurPoint += 3;
                Assert(m_uCurPoint < 12);
                break;
            }

            fSet = (m_uCurIndex < m_pFigure->Count - 1);
            if (fSet)
            {
                pSegment = reinterpret_cast<MilSegment*> 
                    (reinterpret_cast<BYTE*>(m_pCurSegment) + sizeof(MilSegmentArc));
            }
            break;
            
        case MilSegmentType::PolyLine: 
            {
                const MilSegmentPoly *pPolySegment = static_cast<MilSegmentPoly*> (m_pCurSegment);

                UINT uLastInnerIndex = pPolySegment->Count - 1;  
                Assert(pPolySegment->Count != 0);
                fSet = (m_uInnerIndex < uLastInnerIndex);
                if (fSet)
                {
                    // We'll move to the next inner segment
                    break;
                }

                fSet = (m_uCurIndex < m_pFigure->Count - 1);
                if (fSet)
                {
                    pSegment = reinterpret_cast<MilSegment*> 
                        (reinterpret_cast<BYTE*>(m_pCurSegment) 
                        + sizeof(MilSegmentPoly)
                        + sizeof(MilPoint2D)*pPolySegment->Count);
                }
                break;
            }
            
        case MilSegmentType::PolyBezier: 
            {
                const MilSegmentPoly *pPolySegment = static_cast<MilSegmentPoly*> (m_pCurSegment);
                Assert((pPolySegment->Count % 3) == 0);

                UINT uLastInnerIndex = pPolySegment->Count / 3 - 1;
                Assert(pPolySegment->Count >= 3);
                
                fSet = (m_uInnerIndex < uLastInnerIndex);
                if (fSet)
                {
                    // We'll move to the next inner segment
                    break;
                }

                fSet = (m_uCurIndex < m_pFigure->Count - 1);
                if (fSet)
                {
                    pSegment = reinterpret_cast<MilSegment*> 
                        (reinterpret_cast<BYTE*>(m_pCurSegment) 
                        + sizeof(MilSegmentPoly) 
                        + sizeof(MilPoint2D)*pPolySegment->Count);
                }
                break;
            }
            
      case MilSegmentType::PolyQuadraticBezier: 
            {
                const MilSegmentPoly *pPolySegment = static_cast<MilSegmentPoly*> (m_pCurSegment);
                Assert((pPolySegment->Count & 1) == 0);
                Assert(pPolySegment->Count >= 2);

                UINT uLastInnerIndex = pPolySegment->Count / 2 - 1;
                
                fSet = (m_uInnerIndex < uLastInnerIndex);
                if (fSet)
                {
                    // We'll move to the next inner segment
                    break;
                }

                fSet = (m_uCurIndex < m_pFigure->Count - 1);
                if (fSet)
                {
                    pSegment = reinterpret_cast<MilSegment*> 
                        (reinterpret_cast<BYTE*>(m_pCurSegment)
                        + sizeof(MilSegmentPoly)
                        + sizeof(MilPoint2D)*pPolySegment->Count);
                }
                break;
             }

      default:
        fSet = false;
        Assert(FALSE);
        break;
    }

    if (fSet)
    {
        if (pSegment)
        {
            m_pCurSegment = pSegment;
            m_uCurIndex++;
            if (MilSegmentType::Arc == pSegment->Type)
            {
                SetArcData();
                m_uCurPoint = 0;
            }

            m_uInnerIndex = 0;
        }
        else
        {
            m_uInnerIndex++;
        }
    }
    else
    {
        // Last chance - the implied closing line segment, if not redundant
        if (IsClosed() && (m_uCurIndex < m_pFigure->Count))
        {
            const MilPoint2D &ptStart = m_pFigure->StartPoint;
            const MilPoint2D &ptCurrent = *GetSegmentLastPoint(m_pCurSegment);
            fSet = ((ptStart.X != ptCurrent.X)  ||  (ptStart.Y != ptCurrent.Y));
            m_uCurIndex++;
        }
    }

    return fSet;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::SetToLastSegment
//
//  Synopsis:
//      Sets to the last segment.
//
//  Returns:
//      True if set
//
//  Note:
//      This method is not tested, as there is no path code that drives it. It
//      will be needed once line-shapes are exposed in the public API.
//
//------------------------------------------------------------------------------
bool PathFigureData::SetToLastSegment() const
{
#ifdef LINE_SHAPES_ENABLED
    Assert(m_pFigure != NULL);

    //
    // This method ignores the implied closing line segment of a closed figure.
    // That's currently acceptable, since backward traversal is only used for
    // line shapes, which are only applied to open figures.
    //
    Assert(!IsClosed());

    bool fSet = (m_pFigure->Count > 0);
    if (fSet)
    {
        Assert(m_pCurSegment != NULL);
        m_pCurSegment = GetLastSegment();
        m_uCurIndex = m_pFigure->Count - 1;
        
        if (MilSegmentType::Arc == m_pCurSegment->Type)
        {
             SetArcData();
        }

        SetInnerIndexToLast();
    }

Cleanup:    
    return fSet;
#else
    RIP("Invalid call");
    return false;
#endif // LINE_SHAPES_ENABLED
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      PathFigureData::SetToPreviousSegment
//
//  Synopsis:
//      Traverse to the previous segment in the figure.
//
//  Returns:
//      True if set
//
//  Note:
//      This method is not tested, as there is not path code that drives it. It
//      will be needed once line-shapes are exposed in the public API.
//
//------------------------------------------------------------------------------
bool PathFigureData::SetToPreviousSegment() const
{
#ifdef LINE_SHAPES_ENABLED

    if (m_uInnerIndex > 0)
    {
        // Decrement the sub-segment
        m_uInnerIndex--;
        if (MilSegmentType::Arc == m_pCurSegment->Type)
        {
            // Move to the arc's previous Bezier segment 
            m_uCurPoint -= 3;
        }
        return true;
    }

    if (m_uCurIndex > 0)
    {
        // Decrement the segment
        m_uCurIndex--;

        Asseret(m_pCurSegment);
        m_pCurSegment = reinterpret_cast<MilSegment*> 
            (reinterpret_cast<BYTE*>(m_pCurSegment) - m_pCurSegment->BackSize);

        if (MilSegmentType::Arc == m_pCurSegment->Type)
        {
            // Compute the arc's Bezier segments 
            SetArcData();
        }

        SetInnerIndexToLast();
        return true;
    }

    return false;
#else
    RIP("Invalid call");
    return false;
#endif // LINE_SHAPES_ENABLED
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::PathGeometryData
//
//  Synopsis:
//      Construct an empty PathGeometryData
//
//------------------------------------------------------------------------------
PathGeometryData::PathGeometryData()
{
    SetPathData(NULL, 0, MilFillMode::Alternate);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::PathGeometryData
//
//  Synopsis:
//      Constructor for PathGeometryData that initializes its content.
//
//------------------------------------------------------------------------------
PathGeometryData::PathGeometryData(
    MilPathGeometry *pPathData, 
    UINT nSize, 
    MilFillMode::Enum fillRule, 
    CMILMatrix *pMatrix)
{
    SetPathData(pPathData, nSize, fillRule, pMatrix);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::SetPathData
//
//  Synopsis:
//      Initialize the path data content.
//
//------------------------------------------------------------------------------
void PathGeometryData::SetPathData(
    MilPathGeometry *pPathData, 
    UINT nSize, 
    MilFillMode::Enum fillRule, 
    const CMILMatrix *pMatrix)
{
    m_pPath = pPathData;
    m_nSize = nSize;

    m_fillRule = fillRule;
    m_pMatrix = pMatrix;
    
    m_uCurIndex = 0;
    m_pCurFigure = GetFirstFigure();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::HasGaps
//
//  Synopsis:
//      Returns true if figure has gaps.
//
//------------------------------------------------------------------------------
bool PathGeometryData::HasGaps() const
{
    Assert(m_pPath);
    return (m_pPath->Flags & MilPathGeometryFlags::HasGaps) != 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::HasHollows
//
//  Synopsis:
//      Returns true if figure has non-fillable figures.
//
//------------------------------------------------------------------------------
bool PathGeometryData::HasHollows() const
{
    Assert(m_pPath != NULL);
    return (m_pPath->Flags & MilPathGeometryFlags::HasHollows) != 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::IsEmpty
//
//  Synopsis:
//      Returns if the path geomety is empty.
//
//------------------------------------------------------------------------------
bool PathGeometryData::IsEmpty() const
{
    Assert(m_pPath != NULL);

    MilPathFigure *pFigure = GetFirstFigure();
    UINT count = m_pPath->FigureCount;

    while (count--)
    {
        PathFigureData pathFigure(pFigure, pFigure->Size, m_pMatrix);
        if (!pathFigure.IsEmpty())
        {
            return false;
        }     

        pFigure = reinterpret_cast<MilPathFigure*>
            (reinterpret_cast<BYTE*>(pFigure) + pFigure->Size);
    }

    return true;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::GetFigureCount
//
//  Synopsis:
//      Returns the number of figures in the path geometry.
//
//------------------------------------------------------------------------------
UINT PathGeometryData::GetFigureCount() const
{
    Assert(m_pPath != NULL);
    return m_pPath->FigureCount;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::GetFigure
//
//  Synopsis:
//      Returns the figure at the given index.
//
//------------------------------------------------------------------------------
const IFigureData &PathGeometryData::GetFigure(IN UINT index) const
{
    Assert(m_pPath != NULL);
    Assert(index <= GetFigureCount());

    if (index != m_uCurIndex)
    {
        if (index == 0)
        {
            m_pCurFigure = GetFirstFigure();
            m_uCurIndex = 0;
        }
        else
        {
            while (m_uCurIndex < index)
            {
                if (!NextFigure()) return *reinterpret_cast<IFigureData*>(NULL);
            }
            
            while (m_uCurIndex > index)
            {
                if (!PrevFigure()) return *reinterpret_cast<IFigureData*>(NULL);
            }
        }
    }

    m_pathFigure.SetFigureData(m_pCurFigure, m_pCurFigure->Size, m_pMatrix);
    
    return m_pathFigure;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::NextFigure
//
//  Synopsis:
//      Traverse forward to the next figure.
//
//------------------------------------------------------------------------------
bool PathGeometryData::NextFigure() const
{
    if (m_uCurIndex >= m_pPath->FigureCount)
    {
        return false;
    }
    
    m_pCurFigure = reinterpret_cast<MilPathFigure*>
        (reinterpret_cast<BYTE*>(m_pCurFigure) + m_pCurFigure->Size);
    m_uCurIndex++;
        
    return true;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::PrevFigure
//
//  Synopsis:
//      Traverse backward to the previous figure.
//
//------------------------------------------------------------------------------
bool PathGeometryData::PrevFigure() const
{
    if (m_uCurIndex <= 0)
    {
        return false;
    }
    
    m_pCurFigure = reinterpret_cast<MilPathFigure*>
        (reinterpret_cast<BYTE*>(m_pCurFigure) - m_pCurFigure->BackSize);
    m_uCurIndex--;
        
    return true;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::GetFillMode
//
//  Synopsis:
//      Return the fill mode.
//
//------------------------------------------------------------------------------
MilFillMode::Enum PathGeometryData::GetFillMode() const
{
    Assert(m_pPath != NULL);

    if (m_fillRule == MilFillMode::Alternate)
    {
        return MilFillMode::Alternate;
    }

    Assert(m_fillRule == MilFillMode::Winding);
    
    return MilFillMode::Winding;
}

bool PathGeometryData::IsAxisAlignedRectangle() const
{
    return FALSE;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::GetCachedBoundsCore
//
//  Synopsis:
//      Get the cached bounds if they exist
//
//  Returns:
//      true if bounds have previously been cached
//
//------------------------------------------------------------------------------
bool
PathGeometryData::GetCachedBoundsCore(
    __out_ecount(1) MilRectF &rect) const    // The cached bounds, set only if valid
{
    Assert(m_pPath);

    bool fCached = ((m_pPath->Flags & MilPathGeometryFlags::BoundsValid) != 0);
    if (fCached)
    {
        MilRectFFromMilRectD(OUT rect, m_pPath->Bounds);
    }

    return fCached;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      PathGeometryData::SetCachedBounds
//
//  Synopsis:
//      Set the cached bounds
//
//------------------------------------------------------------------------------
void
PathGeometryData::SetCachedBounds(
    __in_ecount(1) const MilRectF &rect) const   // The bounding box to cache
{
    Assert(m_pPath);

    MilRectDFromMilRectF(OUT m_pPath->Bounds, rect);
    
    m_pPath->Flags |= MilPathGeometryFlags::BoundsValid;
}



