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
//      Definition of the IShapeData interface.
//
//  $ENDTAG
//
//  Classes:
//      IShapeData, IFigureData
//
//------------------------------------------------------------------------------

// Rename IShapeData to CShapeBase & remove typedef
typedef CShapeBase IShapeData;

//+-----------------------------------------------------------------------------
//
//  Class:
//      IFigureData
//
//  Synopsis:
//      Interface for access and queries on figure data
//
//------------------------------------------------------------------------------
class IFigureData
{
public:
    // Constructor/destructor
    IFigureData()
    {
    }

    virtual ~IFigureData()
    {
    }

    // Properties
    virtual bool IsEmpty() const = 0;

    virtual bool HasNoSegments() const = 0;

    virtual HRESULT GetCountsEstimate(
        __out_ecount(1) UINT &cSegments,    // A bound on the nunber of segments 
        __out_ecount(1) UINT &cPoints       // A bound on the number of points
        ) const = 0;

    virtual bool IsClosed() const = 0;

    virtual bool IsAtASmoothJoin() const = 0;

    // At the last segment the join in question is with the first segment 
    // if the figure is closed, otherwise the method should return false.

    virtual bool HasGaps() const = 0;

    virtual bool IsAtAGap() const = 0;

    virtual bool IsFillable() const = 0;

    // Rectangle optimization
    virtual bool IsAParallelogram() const = 0;

    virtual bool IsAxisAlignedRectangle() const = 0;

    virtual VOID GetAsRectangle(__out_ecount(1) MilRectF &rect) const = 0;

    virtual VOID GetAsWellOrderedRectangle(__out_ecount(1) MilRectF &rect) const = 0;

    virtual VOID GetParallelogramVertices(
        __out_ecount_full(4) MilPoint2F      *pVertices,
            // 4 Parallelogram's vertices
        __in_ecount_opt(1) const CMILMatrix  *pMatrix=NULL
            // Transformation (optional)
        ) const = 0;
    
    virtual VOID GetRectangleCorners(
        __out_ecount_full(2) MilPoint2F *pCorners
            // 2 Rectangle's corners
        ) const = 0;

    // Traversal
    virtual bool SetToFirstSegment() const = 0;
    
    virtual bool GetCurrentSegment(     // Returns true if this is the last segment
        __out_ecount(1) BYTE            &bType,
            // Segment type (line or Bezier)
        __deref_outro_xcount((bType == MilCoreSeg::TypeLine) ? 1 : 3) const MilPoint2F *&pt
            // Line endpoint or curve 3 last points
        ) const = 0;

    virtual bool SetToNextSegment() const = 0;

    virtual __outro_ecount(1) const MilPoint2F &GetCurrentSegmentStart() const = 0;

    virtual void SetStop() const = 0;
    
    virtual void ResetStop() const = 0;

    virtual bool IsStopSet() const = 0;

    // Start & end points
    virtual __outro_ecount(1) const MilPoint2F &GetStartPoint() const = 0;
    virtual __outro_ecount(1) const MilPoint2F &GetEndPoint() const = 0;
    
    // These two functions are only needed for milcoretest.dll. They will have
    // no implementations otherwise. Their declarations are ncluded so that
    // we don't need to compile all source files that include this one twice-
    // (once for milcoretest and once for milcore)
    virtual bool SetToLastSegment() const = 0;
    virtual bool SetToPreviousSegment() const = 0;

};

