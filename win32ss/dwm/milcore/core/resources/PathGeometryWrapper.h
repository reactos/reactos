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
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilPathGeometryDuce);

//+-----------------------------------------------------------------------------
//
//  Class:
//      PathFigureData
//
//  Synopsis:
//      Interface for access and queries on figure data
//
//------------------------------------------------------------------------------
class PathFigureData : public IFigureData
{
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilPathGeometryDuce));

public:
    PathFigureData();
    
    PathFigureData(MilPathFigure *pFigure, UINT nSize,  const CMILMatrix *pMatrix = NULL);
    VOID SetFigureData(MilPathFigure *pFigure, UINT nSize, const CMILMatrix *pMatrix = NULL);
    
    void SetInnerIndexToLast() const;

    // IShapdData overrides
    virtual bool IsEmpty() const;
    virtual bool HasNoSegments() const;
    virtual HRESULT GetCountsEstimate(
        OUT UINT &cSegments,    // A bound on the nunber of segments 
        OUT UINT &cPoints       // A bound on the number of points
        ) const;

    virtual bool IsClosed() const;
    virtual bool IsAtASmoothJoin() const;
    virtual bool HasGaps() const;    
    virtual bool IsAtAGap() const;
    virtual bool IsFillable() const;
    virtual bool IsAParallelogram() const 
    {
        return (m_pFigure->Flags & MilPathFigureFlags::IsRectangleData) != 0; 
    }
    virtual bool IsAxisAlignedRectangle() const;
    virtual VOID GetAsRectangle(__out_ecount(1) MilRectF &rect) const;
    virtual VOID GetAsWellOrderedRectangle(__out_ecount(1) MilRectF &rect) const;
    virtual VOID  GetParallelogramVertices(
        __out_ecount(4) MilPoint2F *pVertices,
            // 4 Rectangle's vertices
        __in_ecount_opt(1) const CMILMatrix *pMatrix=NULL
            // Transformation (optional)
    ) const;
    virtual VOID GetRectangleCorners(
        OUT MilPoint2F *pCorners       // 2 Rectangle's corners
    ) const  {  RIP("Unsupported call"); }
    virtual bool SetToFirstSegment() const;    
    virtual bool SetToNextSegment() const;
    virtual bool GetCurrentSegment(     // Returns true if this is the last segment
        __out_ecount(1) BYTE            &bType,     // Segment type (line or Bezier)
        __deref_outro_xcount((bType == MilCoreSeg::TypeLine) ? 1 : 3) const MilPoint2F *&pt        // Line endpoint or curve 3 last points
        ) const;
    virtual const MilPoint2F &GetCurrentSegmentStart() const;
    virtual void SetStop() const
    {
        m_uStop = m_uCurIndex;
        m_uInnerStop = m_uInnerIndex;
    }
    virtual void ResetStop() const
    {
        m_uStop = m_uInnerStop = UINT_MAX;
    }
    virtual bool IsStopSet() const
    {
        return (m_uStop < UINT_MAX)  ||  (m_uInnerStop < UINT_MAX);
    }
    virtual const MilPoint2F &GetStartPoint() const;
    virtual const MilPoint2F &GetEndPoint() const;

    // These two functions are only needed for milcoretest.dll. They will have
    // no implementations otherwise. Their declarations are ncluded so that
    // we don't need to compile all source files that include this one twice-
    // (once for milcoretest and once for milcore)
    virtual bool SetToLastSegment() const;
    virtual bool SetToPreviousSegment() const;
    
private:
    // Private helper methods
    VOID SetQuadraticBezier(
        IN MilPoint2D pt0,
        IN MilPoint2D pt1,
        IN MilPoint2D pt2) const;
    
    __notnull const MilPoint2D *GetCurrentSegmentStartD() const;
    __notnull MilPoint2D *GetSegmentLastPoint(MilSegment *pSegment) const;
    MilSegment *GetSegment(INT index) const;

    bool NextSegment() const;
    bool PrevSegment() const;   

    inline MilSegment *GetFirstSegment() const
    {
        return reinterpret_cast<MilSegment*>(m_pFigure+1);
    }

    inline MilSegment *GetLastSegment() const
    {
        return reinterpret_cast<MilSegment*>
            (reinterpret_cast<BYTE*>(m_pFigure) + m_pFigure->OffsetToLastSegment);
    }

    void SetArcData() const;

private:
    // Raw shape data
    MilPathFigure *m_pFigure;
    INT m_nSize;
    const CMILMatrix *m_pMatrix;
    
    // Iteration state
    mutable MilSegment *m_pCurSegment;
    mutable UINT m_uInnerIndex;
    mutable UINT m_uCurIndex;

    // Stop index
    mutable UINT m_uStop;
    mutable UINT m_uInnerStop;

    // Scratch area for returned points
    mutable MilPoint2F m_rgPoints[12];
    // Cannot use m_rgPoints start and end because it may be loaded with arc points
    mutable MilPoint2F m_ptStartPoint;
    mutable MilPoint2F m_ptEndPoint;
    mutable bool m_fEndPointValid;

    // Specific arc data, not used for other types
    mutable BYTE m_bType;
    mutable UINT m_uCurPoint;
    mutable UINT m_uLastInnerIndex;

};  // End of definition of PathFigureData

//+-----------------------------------------------------------------------------
//
//  Class:
//      PathGeometryData
//
//  Synopsis:
//      Implements IShapeData
//
//------------------------------------------------------------------------------

class PathGeometryData : public IShapeData
{
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilPathGeometryDuce));

    // Constructor/destructor
    PathGeometryData();
    PathGeometryData(
        MilPathGeometry *pPathData, 
        UINT nSize, 
        MilFillMode::Enum fillRule, 
        CMILMatrix *pMatrix=NULL);    

    void SetPathData(
        MilPathGeometry *pPathData, 
        UINT nSize, 
        MilFillMode::Enum fillRule, 
        const CMILMatrix *pMatrix=NULL);
        
    // IShapeData overrides
    virtual bool HasGaps() const;    
    virtual bool HasHollows() const;
    virtual bool IsEmpty() const;
    virtual UINT GetFigureCount() const;
    virtual const IFigureData &GetFigure(IN UINT index) const;
    virtual MilFillMode::Enum GetFillMode() const;
    virtual bool IsAxisAlignedRectangle() const;

    virtual bool IsARegion() const
    {
        return (m_pPath->Flags & MilPathGeometryFlags::IsRegionData) != 0;
    }

    // Other methods
    bool NextFigure() const;
    bool PrevFigure() const;

protected:

    virtual bool GetCachedBoundsCore(
        __out_ecount(1) MilRectF &rect) const;   // The cached bounds, set only if valid

    virtual void SetCachedBounds(
        __in_ecount(1) const MilRectF &rect) const;  // Bounding box to cache   
    
private:
    inline MilPathFigure *GetFirstFigure() const
    {
        return reinterpret_cast<MilPathFigure*> (m_pPath+1);
    }
    
    // The shape data
    MilPathGeometry *m_pPath;
    UINT m_nSize;
    
    MilFillMode::Enum m_fillRule;
    const CMILMatrix *m_pMatrix;
    
    mutable UINT m_uCurIndex;
    mutable MilPathFigure *m_pCurFigure;
    mutable PathFigureData m_pathFigure;
 };

void
MilUtility_GetArcBounds(
    IN MilPoint2D   start,     // start point
    IN MilPoint2D   radius,    // The ellipse'sradius
    IN double               rRotation,  // Rotation angle of the ellipse's x axis
    IN BOOL                 fLargeArc,  // Choose the larger of the 2 possible arcs if TRUE
    IN BOOL                 fSweepUp,   // Sweep the arc while increasing the angle if TRUE
    IN MilPoint2D   end,       // last point
    OUT MilPointAndSizeD   &rect);     // The bounding box

void
MilUtility_GetBezierBounds(
    IN MilPoint2D  &point1,     // point1
    IN MilPoint2D  &point2,     // point2
    IN MilPoint2D  &point3,     // point3
    IN MilPoint2D  &point4,     // point4
    IN OUT MilPoint2D  &minPoint,   // last point
    IN OUT MilPoint2D  &maxPoint);   // last point

void
MilUtility_GetQuadraticBezierBounds(
    IN MilPoint2D  &point1,     // point1
    IN MilPoint2D  &point2,     // point2
    IN MilPoint2D  &point3,     // point3
    IN OUT MilPoint2D  &minPoint,   // last point
    IN OUT MilPoint2D  &maxPoint);   // last point





