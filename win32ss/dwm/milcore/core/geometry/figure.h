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
//      Implementation of the class CFigure, and its derived and helper classes
//      CShape is a list of figures, and it has a fill mode.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

extern const WORD FigureFlagClosed;
extern const WORD FigureFlagCurved;
extern const WORD FigureFlagRectangle;

extern const WORD FigureFlagNoFill;
extern const WORD FigureFlagGapState;
extern const WORD FigureFlagHasGaps;

#if DBG
extern bool g_fTraceFigureConstruction;
#endif



MtExtern(CFigureData);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CFigureData
//
//  Synopsis:
//      Implementation of IFigureData and IFigureBuilder + additional internal
//      methods
//
//------------------------------------------------------------------------------
class CFigureData   :   public IFigureData, public IFigureBuilder
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CFigureData));

    // Disallow copy and =
private:
    CFigureData(__in_ecount(1) const CFigureData &other)
    {
        Assert(false);
    }
    void operator = (__in_ecount(1) const CFigure &other);

public:
    CFigureData()
    : m_bFlags(0)
    {
        m_uCurrentSegment = m_uCurrentPoint = 0;
        m_uStop = UINT_MAX;
    }

    virtual ~CFigureData()
    {
    }

    // IFigureBuilder overrides
    virtual HRESULT StartAt(
        REAL x, REAL y); // In: Figure's start point

    virtual HRESULT LineTo(
        IN   REAL x,              // Line's end point X
        IN   REAL y,              // Line's end point Y
        bool fSmoothJoin=false    // = true if forcing a smooth join
        );

    virtual HRESULT BezierTo(
        IN REAL x2, REAL y2, 
            // Second Bezier point
        IN REAL x3, REAL y3,
            // Third Bezier point
        IN REAL x4, REAL y4,
            // Fourth Bezier point
        bool fSmoothJoin=false); 
            // = true if forcing a smooth join

    virtual HRESULT Close();

    virtual void SetFillable(BOOL fValue)
    {
        if (fValue)
        {
            m_bFlags &= (~FigureFlagNoFill);
        }
        else
        {
            m_bFlags |= FigureFlagNoFill;
        }
    }

    virtual void SetStrokeState(BOOL fValue)
    {
        if (fValue)
        {
            m_bFlags &= (~FigureFlagGapState);
        }
        else
        {
            m_bFlags |= FigureFlagGapState;
        }
    }

    void Reset(bool shrink=FALSE)
    {
        m_bFlags = 0;
        m_rgPoints.Reset(shrink);
        m_rgTypes.Reset(shrink);
    }
   
    // IFigureData overrides
    virtual bool IsEmpty() const
    {
#if DBG
        if (0 == m_rgPoints.GetCount())
        {
            // You can't have a segment without at least one associated point
            Assert(0 == m_rgTypes.GetCount());
        }
#endif
        return (0 == m_rgPoints.GetCount());
    }

    virtual bool HasNoSegments() const
    {
        return m_rgTypes.GetCount() < 1;
    }

    virtual HRESULT GetCountsEstimate(
        __out_ecount(1) UINT &cSegments,
            // An estimate of the nunber of segments 
        __out_ecount(1) UINT &cPoints
            // An estimate of the number of points
        ) const
    {
        cSegments = m_rgTypes.GetCount();
        cPoints = m_rgPoints.GetCount();
        return S_OK;
    }

    virtual bool IsClosed() const
    {
        return 0 != (m_bFlags & FigureFlagClosed);
    }

    virtual bool HasCurve() const
    {
        return 0 != (m_bFlags & FigureFlagCurved);
    }

    virtual bool IsAtASmoothJoin() const
    {
        return (m_rgTypes[m_uCurrentSegment] & MilCoreSeg::SmoothJoin) != 0;
    }

    virtual bool HasGaps() const
    {
        return 0 != (m_bFlags & FigureFlagHasGaps);
    }

    virtual bool IsAtAGap() const
    {
        return (m_rgTypes[m_uCurrentSegment] & MilCoreSeg::IsAGap) != 0;
    }

    virtual bool IsFillable() const
    {
        return 0 == (m_bFlags & FigureFlagNoFill);
    }

    // Rectangle optimization

    // IsAParallelogram  returns true if the figure was initialized as a rectangle.
    // It may have been Transformed to a parallelogram.
    virtual bool IsAParallelogram() const
    {
        return 0 != (m_bFlags & FigureFlagRectangle);
    }

    virtual bool IsAxisAlignedRectangle() const;

    virtual void GetAsRectangle(__out_ecount(1) MilRectF &rect) const;

    virtual void GetAsWellOrderedRectangle(__out_ecount(1) MilRectF &rect) const;

    virtual void GetParallelogramVertices(
        __out_ecount_full(4) MilPoint2F       *pVertices,
            // 4 Rectangle's vertices
        __in_ecount_opt(1) const CMILMatrix   *pMatrix=NULL
            // Transformation (optional)
        ) const;

    virtual void GetRectangleCorners(
        __out_ecount_full(2) MilPoint2F *pVertices       // 2 Rectangle's corners
        ) const;

    // Traversal    
    virtual bool SetToFirstSegment() const;
    
    virtual bool GetCurrentSegment(     // Returns true if this is the last segment
        __out_ecount(1) BYTE            &bType,
            // Segment type (line or Bezier)
        __deref_outro_xcount((bType == MilCoreSeg::TypeLine) ? 1 : 3) const MilPoint2F *&pt
            // Line endpoint or curve 3 last points
        ) const;

    virtual bool SetToNextSegment() const;

    virtual __outro_ecount(1) const MilPoint2F &GetCurrentSegmentStart() const
    {
        Assert(m_rgPoints.GetCount() > 0);
        return m_rgPoints[m_uCurrentPoint - 1];
    }

    virtual __outro_ecount(1) const MilPoint2F &GetStartPoint() const
    {
        Assert(m_rgPoints.GetCount() > 0);
        return m_rgPoints[0];
    }
    
    virtual __outro_ecount(1) const MilPoint2F &GetEndPoint() const
    {
        Assert(m_rgPoints.GetCount() > 0);
        return m_rgPoints.Last();
    }

    // These two functions are only needed for milcoretest.dll. They will have
    // no implementations otherwise. Their declarations are ncluded so that
    // we don't need to compile all source files that include this one twice-
    // (once for milcoretest and once for milcore)
    virtual bool SetToLastSegment() const;
    virtual bool SetToPreviousSegment() const;

    virtual void SetStop() const
    {
        m_uStop = m_uCurrentSegment;
    }
    
    virtual void ResetStop() const
    {
        m_uStop = UINT_MAX;
    }

    virtual bool IsStopSet() const
    {
        return m_uStop < UINT_MAX;
    }

    // Utilities
    void Transform(
        __in_ecount(1) const CBaseMatrix &matrix
            // Transformation matrix
        );

    void SetAsRectangle()
    {
        m_bFlags |= FigureFlagRectangle;
    }

    HRESULT SetFrom(
        __in_ecount(1) const IFigureData   &other,          // The data to copy
        __in_ecount_opt(1) const CMILMatrix *pMatrix=NULL); // Transformation to apply to the input

    HRESULT Copy(
        __in_ecount(1) const CFigureData &other);
            // The data to copy

    HRESULT AddSegments(
    __in int                             cPoints,        // Number of points to add
    __in int                             cSegs,          // Number of segment to add
    __in_ecount(cPoints) const MilPoint2F *pPt,           // Array of points to add
    IN MilCoreSeg::Flags                   eSegType,       // The type of segments
    IN bool                              fSmooth=false); // Enforce smoothness at joins if true

    HRESULT AddPoints(
        IN int cPoints,
            // Number of points to add
        __deref_out_ecount_part(cPoints, 0) MilPoint2F   *&pData)
            // Pointer to the first new added point 
    {
        return m_rgPoints.AddMultiple(cPoints, &pData);
    }

    HRESULT AddPoint(MilPoint2F pt)
    {
        return m_rgPoints.Add(pt);
    }

    HRESULT AddTypes(
        IN int     count,
            // The number of types to add
        __deref_out_ecount_part(count, 0) BYTE   *&pData)
            // Pointer to the first new added type
    {
        Assert(count > 0);
        if (0 != (FigureFlagGapState & m_bFlags))
        {
            m_bFlags |= FigureFlagHasGaps;
        }
        return m_rgTypes.AddMultiple(count, &pData);
    }

    HRESULT AddAndSetTypes(
        IN int             count,          // The number of types to add
        IN MilCoreSeg::Flags type,           // Their values
        IN bool            fSmooth=false); // Enforce smoothness at corner if true (optional)

    void SetClosed()
    {
        // Should only be called if you know that, otherwise use the Close()
        Assert(MilPoint2LsEqualOrNaNs(GetStartPoint(), GetEndPoint()));
        m_bFlags |= FigureFlagClosed;
    }

    void Reverse();

    // State that this figure contains curves
    VOID SetCurved()
    {
        m_bFlags |= FigureFlagCurved;
    }

    // Constructions
    HRESULT StartAtPtR(
        __in_ecount(1) const GpPointR UNALIGNED &pt)
            // In: Figure's start point
    {
        return StartAt(REAL(pt.X), REAL(pt.Y));
    }

    HRESULT StartAtPtF(
        __in_ecount(1) const MilPoint2F UNALIGNED &pt)
            // In: Figure's start point
    {
        return StartAt(pt.X, pt.Y);
    }

    HRESULT LineToPtR(
        __in_ecount(1) const GpPointR &pt)
            // In: Line's end point
    {
        return LineTo(REAL(pt.X), REAL(pt.Y));
    }

    HRESULT LineToPtF(
        __in_ecount(1) const MilPoint2F UNALIGNED &pt)
            // Line's end point
    {
        return LineTo(pt.X, pt.Y);
    }

    HRESULT LinesTo(
        __in_ecount(count) const MilPoint2F *rgPoints,
            // Array of points to add
        IN INT count);
            // Number of points

    HRESULT BezierToPtR(
        __in_ecount(1) const GpPointR &pt1,
            // Second Bezier point
        __in_ecount(1) const GpPointR &pt2,
            // Third Bezier point
        __in_ecount(1) const GpPointR &pt3)
            // Last Bezier point
        {
            return BezierTo(REAL(pt1.X),
                            REAL(pt1.Y),
                            REAL(pt2.X),
                            REAL(pt2.Y),
                            REAL(pt3.X),
                            REAL(pt3.Y));
        }

    HRESULT BezierToPtF(
        __in_ecount(1) const MilPoint2F &pt1,
            // Second Bezier point
        __in_ecount(1) const MilPoint2F &pt2,
            // Third Bezier point
        __in_ecount(1) const MilPoint2F &pt3)
            // Last Bezier point
        {
            return BezierTo(pt1.X, pt1.Y, pt2.X, pt2.Y, pt3.X, pt3.Y);
        }

    HRESULT BeziersTo(
        __in_ecount(count) const MilPoint2F *rgPoints,
            // Array of points
        IN INT count);
            // Number of points

    HRESULT ArcTo(
        IN FLOAT xRadius,    // The ellipse's X radius
        IN FLOAT yRadius,    // The ellipse's Y radius
        IN FLOAT rRotation,  // Rotation angle of the ellipse's x axis
        IN BOOL  fLargeArc,  // Choose the larger of the 2 possible arcs if TRUE
        IN BOOL  fSweepUp,   // Sweep the arc while increasing the angle if TRUE
        IN FLOAT xEnd,       // X coordinate of the last point
        IN FLOAT yEnd);      // Y coordinate of the last point

    // Batch constructions
    //   The figure must be empty before calling these.
    HRESULT InitAsRectangle(
        __in_ecount(1) const CMilRectF &rect);      // The rectangle

    HRESULT InitAsEllipse(
        REAL rCenterX, REAL rCenterY,   // The center of the ellipse
        REAL rRadiusX,                  // The X-radius of the ellipse
        REAL rRadiusY                   // The Y-radius of the ellipse        
        );

    HRESULT InitAsRoundedRectangle(
        __in_ecount(1) const CMilRectF &rect,
            // The bounding rectangle
        IN REAL rRadiusX,
            // The X-radius of the corner (elliptical arc)
        IN REAL rRadiusY
            // The Y-radius of the corner (elliptical arc)
        );

    HRESULT InitAsBeveledRectangle(
        __in_ecount(1) const CMilRectF &rect,
            // The bounding rectangle
        IN REAL rBevelOffset
        );

    // Quick and dirty way to get at the points
    __xcount(GetPointCount()) MilPoint2F *GetRawPoints() const
    {
        return m_rgPoints.GetDataBuffer();    
    }

    UINT GetPointCount() const
    {
        return m_rgPoints.GetCount();
    }

    // Quick and dirty way to get at the types
    __xcount(GetSegCount()) BYTE* GetRawTypes() const
    {
        return m_rgTypes.GetDataBuffer();
    }

    UINT GetSegCount() const
    {
        return m_rgTypes.GetCount();
    }

    HRESULT InitFromRawData(
        __in UINT cPoints,
            // Point count
        __in UINT cSegments,
            // Segment count
        __in_ecount(cPoints) MilPoint2D *pPoints,
            // Points
        __in_ecount(cSegments) byte *pTypes,
            // Types
        __in_ecount_opt(1) CMILMatrix *pMatrix = NULL);
            // A transformation to apply to the points

    static void InitBufferWithRectanglePoints(
        __out_ecount_full(4) MilPoint2F *pts,
            // The point buffer
        __in_ecount(1) const MilRectF &rect
            // The rectangle
        );

    static void InitBufferWithRoundedRectanglePoints(
        __out_ecount_full(16) MilPoint2F *pts,
            // The point buffer
        __in_ecount(1) const CMilRectF &rect,
            // The rectangle
        REAL rRadiusX,
            // The X-radius of the corner (elliptical arc)
        REAL rRadiusY
            // The Y-radius of the corner (elliptical arc)
        );
        
protected:
    inline BYTE GetType(UINT u) const
    {
        return static_cast<BYTE>(m_rgTypes[u] & MilCoreSeg::TypeMask);
    }

   // Data
    // The numbers are set so that a single rectangle/ellipse/circle will fit
    // within the initial allocation on the stack.
    DynArrayIA<MilPoint2F, 13>    m_rgPoints;     // Points
    DynArrayIA<BYTE, 4>          m_rgTypes;      // Types
    WORD                         m_bFlags;       // Packed properties

    // Traversal
    mutable UINT                 m_uCurrentSegment;   // The current segment
    mutable UINT                 m_uCurrentPoint;     // The start point of the current segment
    mutable UINT                 m_uStop;             // Segment index to stop at
};


MtExtern(CFigure);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CFigure
//
//  Synopsis:
//      Implementation of CFigureBase as a wrappr aroung CFigureData
//
//------------------------------------------------------------------------------

class CFigure   :   
    public IFigureBuilder,
    public CFigureBase
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CFigure));

    // Disallow assignment
private:
    // This should not be used. Use Clone() instead.
    // So we declare it, but don't define it. Attempts at calling it will cause a link error.

    void operator = (const CFigure &other);

public:
    CFigure()
        : CFigureBase(m_oData)
    {
    }

    virtual ~CFigure()
    {
    }

    // IFigureBuilder overrides
    virtual HRESULT StartAt(
        REAL x, REAL y) // In: Figure's start point
    {
        return m_oData.StartAt(x, y);
    }

    virtual HRESULT LineTo(
        IN   REAL x,              // Line's end point X
        IN   REAL y,              // Line's end point Y
        bool fSmoothJoin=false    // = true if forcing a smooth join
        )
    {
        return m_oData.LineTo(x, y, fSmoothJoin);
    }

    virtual HRESULT BezierTo(
        IN REAL x2, REAL y2, 
            // Second Bezier point
        IN REAL x3, REAL y3,
            // Third Bezier point
        IN REAL x4, REAL y4,
            // Fourth Bezier point
        bool fSmoothJoin=false) 
            // = true if forcing a smooth join
    {
        return m_oData.BezierTo(x2, y2, x3, y3, x4, y4, fSmoothJoin);
    }

    virtual HRESULT Close()
    {
        return m_oData.Close();
    }

    virtual void SetStrokeState(BOOL fValue)
    {
        m_oData.SetStrokeState(fValue);
    }

    VOID SetFillable(BOOL fValue)
    {
        m_oData.SetFillable(fValue);
    }

    VOID Reset(bool shrink=FALSE)
    {
        m_oData.Reset(shrink);
    }

    // Other methods

    virtual bool IsClosed() const
    {
        return m_oData.IsClosed();
    }

    virtual __outro_ecount(1) const MilPoint2F &GetEndPoint() const
    {
        return m_oData.GetEndPoint();
    }

    __outro_ecount(1) const CFigureData &GetData() const
    {
        return m_oData;
    }

    HRESULT Clone(
        __deref_out_ecount(1) CFigure *& pClone) const;
            // The clone

    VOID SetStrokeState(bool fValue)
    {
        m_oData.SetStrokeState(fValue);
    }

    HRESULT BeziersTo(
        __in_ecount(count) const MilPoint2F *rgPoints,
            // Array of points
        IN INT count)
            // Number of points
    {
        return m_oData.BeziersTo(rgPoints, count);
    }

    HRESULT LinesTo(
        __in_ecount(count) const MilPoint2F *rgPoints,
            // Array of points to add
        IN INT count)
            // Number of points
    {
        return m_oData.LinesTo(rgPoints, count);
    }

    HRESULT ArcTo(
        IN FLOAT xRadius,    // The ellipse's X radius
        IN FLOAT yRadius,    // The ellipse's Y radius
        IN FLOAT rRotation,  // Rotation angle of the ellipse's x axis
        IN BOOL  fLargeArc,  // Choose the larger of the 2 possible arcs if TRUE
        IN BOOL  fSweepUp,   // Sweep the arc while increasing the angle if TRUE
        IN FLOAT xEnd,       // X coordinate of the last point
        IN FLOAT yEnd)       // Y coordinate of the last point
    {
        return m_oData.ArcTo(xRadius, yRadius, rRotation, fLargeArc, fSweepUp, xEnd, yEnd);
    }

    // Modifications
    void Reverse()
    {
        m_oData.Reverse();
    }

    // Data
protected:
    CFigureData     m_oData;     // The data
};




