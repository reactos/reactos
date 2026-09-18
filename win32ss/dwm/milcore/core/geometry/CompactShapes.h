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
//      Definition of CCompactShape and its subclasses CParallelogram, CLine
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CParallelogram);
MtExtern(CRectangle);

#if DBG
    #define COMPACT_VALID_FLAG bool m_fDbgValid
    #define SET_COMPACT_VALID(v) m_fDbgValid=(v)
    #define ASSERT_COMPACT_VALID Assert(m_fDbgValid)
#else
    #define COMPACT_VALID_FLAG
    #define SET_COMPACT_VALID
    #define ASSERT_COMPACT_VALID
#endif
    
//+-----------------------------------------------------------------------------
//
//  Class:
//      CCompactFigure
//
//  Synopsis:
//      A base class for the compact representations of commonly used primitives
//      with no gaps
//
//------------------------------------------------------------------------------
class CCompactFigure: public IFigureData
{
public:
    // Constructor/destructor
    CCompactFigure()
    {
        SET_COMPACT_VALID(false);
    }

    virtual ~CCompactFigure()
    {
    }

    // IFigureData overrides
    virtual bool IsEmpty() const
    {
        return false;
    }

    virtual bool HasNoSegments() const
    {
        return false;
    }

    virtual bool IsAtASmoothJoin() const
    {
        return false;
    }

    virtual bool HasGaps() const
    {
        return false;
    }

    virtual bool IsAtAGap() const
    {
        return false;
    }

    // The Stop methods are only used by CLineShape, which only apply to open figures,
    // and are meaningless for a line.  Their implementation therefore does nothing
    virtual void SetStop() const
    {
    }
    
    virtual void ResetStop() const
    {
    }

    virtual bool IsStopSet() const
    {
        return false;
    }

    // Debug-only valid flag
    COMPACT_VALID_FLAG;

protected:
    
    inline static void ComputeBoundsOfPoints(
        __in_ecount(cPoints) const MilPoint2F *points,
        __in UINT cPoints,
        __out_ecount(1) MilRectF &rect
        );    
};
//+-----------------------------------------------------------------------------
//
//  Class:
//      CCompactShape
//
//  Synopsis:
//      A base class for the compact representations of commonly used
//      single-figure shapes
//
//------------------------------------------------------------------------------
class CCompactShape: public IShapeData
{
public:
    // Constructor/destructor
    CCompactShape()
    {
    }

    virtual ~CCompactShape()
    {
    }

    // IShapeData overrides
    virtual bool HasGaps() const
    {
        return false;
    }
    
    virtual bool HasHollows() const
    {
        return false;
    }
    
    virtual bool IsEmpty() const
    {
        return false;
    }

    virtual UINT GetFigureCount() const
    {
        return 1;
    }

    virtual MilFillMode::Enum GetFillMode() const
    {
        // For a single-figure simple shape the fill mode is incosequential
        return MilFillMode::Winding ;
    }

    void SetCachedBounds(
        __in_ecount(1) const MilRectF &rect
        ) const // Ignored here
    {
        // Do nothing, we do not cache the bounds
        UNREFERENCED_PARAMETER(rect);
    }
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CRectangleFigure
//
//  Synopsis:
//      A compact representation of a rounded rectangle figure.
//
//------------------------------------------------------------------------------
class CRectangleFigure : public CCompactFigure
{
    // Constructor/destructor
public:
    CRectangleFigure()
        : m_uCurrentSegment(0)
    {
    }

    virtual ~CRectangleFigure()
    {
    }

    // IFigureData overrides
    virtual HRESULT GetCountsEstimate(
        __out_ecount(1) UINT &cSegments,    // A bound on the number of segments 
        __out_ecount(1) UINT &cPoints       // A bound on the number of points
        ) const
    {
        if (IsAxisAlignedRectangle())
        {
            cSegments = s_uRectNumSegments;
            cPoints = s_uRectNumPoints+1;  // +1 because the first point is double-counted
        }
        else
        {
            cSegments = s_uRoundRectNumSegments;
            cPoints = s_uRoundRectNumPoints+1; // +1 because the first point is double-counted
        }

        return S_OK;
    }

    virtual bool IsClosed() const
    {
        return true;
    }

    virtual bool IsFillable() const
    {
        return true;
    }

    virtual bool IsAParallelogram() const
    {
        return InternalIsAxisAlignedRectangle();
    }

    virtual bool IsAxisAlignedRectangle() const
    {
        return InternalIsAxisAlignedRectangle();
    }

    virtual VOID GetAsRectangle(
        __out_ecount(1) MilRectF &rect
        ) const
    {
        Assert(IsAxisAlignedRectangle());

        GetBounds(rect);
    }

    virtual VOID GetAsWellOrderedRectangle(
        __out_ecount(1) MilRectF &rect
        ) const
    {
        Assert(IsAxisAlignedRectangle());

        GetBounds(rect);
    }

    virtual VOID GetParallelogramVertices(
        __out_ecount(4)  MilPoint2F            *pVertices,    // 4 Parallelogram's vertices
        __in_ecount_opt(1)  const CMILMatrix  *pMatrix=NULL  // Transformation (optional)
        ) const;

    virtual VOID GetRectangleCorners(
        __out_ecount(2)  MilPoint2F *pCorners       // 2 Rectangle's corners
        ) const;

    virtual bool SetToFirstSegment() const
    {
        m_uCurrentSegment = 0;
        return true;
    }
    
    virtual bool GetCurrentSegment(     // Returns true if this is the last segment
        __out_ecount(1) BYTE            &bType,
            // Segment type (line or Bezier)
        __deref_outro_xcount((bType == MilCoreSeg::TypeLine) ? 1 : 3) const MilPoint2F *&pt
            // Line endpoint or curve 3 last points
        ) const;

    virtual bool SetToNextSegment() const;

    virtual __outro_ecount(1) const MilPoint2F &GetCurrentSegmentStart() const;

    virtual __outro_ecount(1) const MilPoint2F &GetStartPoint() const
    {
        ASSERT_COMPACT_VALID;

        return m_pt[0];
    }

    virtual __outro_ecount(1) const MilPoint2F &GetEndPoint() const
    {
        ASSERT_COMPACT_VALID;

        return m_pt[0];
    }
    
    virtual bool SetToLastSegment() const
    {
        ASSERT_COMPACT_VALID;

        if (IsAxisAlignedRectangle())
        {
            m_uCurrentSegment = s_uRectNumSegments-1;
        }
        else
        {
            m_uCurrentSegment = s_uRoundRectNumSegments-1;
        }

        return true;
    }

    virtual bool SetToPreviousSegment() const;

    virtual bool IsAtASmoothJoin() const
    {
        if (IsAxisAlignedRectangle())
        {
            return false;
        }
        else
        {
            return (m_uCurrentSegment % 2 == 0);
        }
    }

    // Other methods
    HRESULT Set(
        __in_ecount(1) const MilPointAndSizeF &rect,
            // A rectangle 
        REAL radius
            // The radiusX/Y value
        )
    {
        HRESULT hr = S_OK;

        Assert(!IsRectEmptyOrInvalid(&rect));

        // Note: This method does no other validation for MilPointAndSizeF's 
        // outside of the LTRB range.        
        CMilRectF rectRB(
            rect.X,
            rect.Y,
            rect.Width,
            rect.Height,
            XYWH_Parameters
            );

        IFC(Set(rectRB, radius));

    Cleanup:
        RRETURN(hr);
    }

    HRESULT Set(
        __in_ecount(1) const MilRectF &rect,
            // A rectangle        
        REAL radius
            // The radiusX/Y value
        );

    void GetBounds(
        __out_ecount(1) MilRectF &rect
        ) const; // The bounds

    REAL GetRadius() const { return m_radius; } 

protected:

    bool InternalIsAxisAlignedRectangle() const
    {
         return m_fHasCorners;
    }

protected:
    static const UINT s_uRoundRectNumSegments = 8;
    static const UINT s_uRectNumSegments = 4;

    static const UINT s_uRoundRectNumPoints = 16;
    static const UINT s_uRectNumPoints = 4;
protected:
    // Data
    CMilPoint2F      m_pt[s_uRoundRectNumPoints];
    REAL m_radius;
    bool m_fHasCorners;
    
    // Traversal
    mutable UINT                 m_uCurrentSegment;   // The current segment
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CRectangle
//
//  Synopsis:
//      A compact representation of a rounded rectangle shape
//
//  NOTE:
//      A CRectangle with radius=0 will be treated as if it were a regular,
//      cornered rectangle. In particular, this means that stroking *will* add
//      miters (or bevels) on to the corners.
//
//------------------------------------------------------------------------------
class CRectangle : public CCompactShape
{
    // Constructor/destructor
public:
    CRectangle()
    {
    }

    virtual ~CRectangle()
    {
    }

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CRectangle));

    // IShapeData overides
    virtual __outro_ecount(1) const IFigureData &GetFigure(IN UINT index) const
    {
        Assert(index == 0);
        return m_figure;
    }

    virtual bool IsAxisAlignedRectangle() const
    {
        return InternalIsAxisAlignedRectangle();
    }

    virtual HRESULT GetTightBounds(__out_ecount(1) CMilRectF &rect) const
    {
        m_figure.GetBounds(rect);
        RRETURN(S_OK);        
    }

    HRESULT GetTightBounds(
        __out_ecount(1) CMilRectF &rect,
            // The bounds of this shape
        __in_ecount_opt(1) const CPlainPen *pPen,
            // The pen (NULL OK but not optional)
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Transformation (NULL OK but not optional)
        __in double rTolerance=0, 
            // Error tolerance (optional)
        __in bool fRelative=false,
            // True if the tolerance is relative (optional)       
        __in bool fSkipHollows=true) const override;
            // If true, skip non-fillable figures when computing fill bounds (optional)       

    HRESULT WidenToShape(
        __in_ecount(1) const            CPlainPen &pen,
            // The pen
        __in double           rTolerance,
            // Flattening tolerance
        __in bool             fRelative,
            // True if the tolerance is relative       
        __in_ecount(1) CShape &widened,
            // The widened shape, populated here
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Transformation matrix (NULL OK)
        __in_ecount_opt(1) const CMILSurfaceRect *prcClip = NULL
            // Clip rectangle (NULL OK)
        ) const;
    
    // Other methods
    HRESULT Set(
        __in_ecount(1) const MilPointAndSizeF &rect,
        REAL radius
        )
    {
        RRETURN(m_figure.Set(rect, radius));
    }

    HRESULT Set(
        __in_ecount(1) const MilRectF &rect,
        REAL radius
        )
    {
        RRETURN(m_figure.Set(rect, radius));
    }

protected:

    bool InternalIsAxisAlignedRectangle() const
    {
        return m_figure.IsAxisAlignedRectangle();
    }

    virtual bool GetCachedBoundsCore(
        __out_ecount(1) MilRectF &rect
        ) const   // The bounds
    {
        m_figure.GetBounds(rect);
        return true;
    }
    
protected:
    // Data
    CRectangleFigure   m_figure;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CParallelogramFigure
//
//  Synopsis:
//      A compact representation of a parallelogram figure
//
//------------------------------------------------------------------------------
class CParallelogramFigure : public CCompactFigure
{
    // Constructor/destructor
public:
    CParallelogramFigure()
    {
    }

    virtual ~CParallelogramFigure()
    {
    }

    // IFigureData overrides
    virtual HRESULT GetCountsEstimate(
        __out_ecount(1) UINT &cSegments,    // A bound on the number of segments 
        __out_ecount(1) UINT &cPoints       // A bound on the number of points
        ) const
    {
        cSegments = 4;
        cPoints = 5;
        return S_OK;
    }

    virtual bool IsClosed() const
    {
        return true;
    }

    virtual bool IsFillable() const
    {
        return true;
    }

    virtual bool IsAParallelogram() const
    {
        return true;
    }

    virtual bool IsAxisAlignedRectangle() const
    {
        return InternalIsAxisAlignedRectangle();
    }

    virtual bool IsARegion() const
    {
        return InternalIsAxisAlignedRectangle();
    }    

    virtual VOID GetAsRectangle(
        __out_ecount(1) MilRectF &rect
        ) const
    {
        GetBounds(rect);
    }

    virtual VOID GetAsWellOrderedRectangle(
        __out_ecount(1) MilRectF &rect
        ) const
    {
        GetBounds(rect);
    }

    virtual VOID GetParallelogramVertices(
        __out_ecount(4)  MilPoint2F            *pVertices,    // 4 Parallelogram's vertices
        __in_ecount_opt(1)  const CMILMatrix  *pMatrix=NULL  // Transformation (optional)
        ) const;
    
    virtual VOID GetRectangleCorners(
        __out_ecount(2)  MilPoint2F *pCorners       // 2 Rectangle's corners
        ) const;

    virtual bool SetToFirstSegment() const
    {
        m_uCurrentSegment = 0;
        return true;
    }
    
    virtual bool GetCurrentSegment(     // Returns true if this is the last segment
        __out_ecount(1) BYTE            &bType,
            // Segment type (line or Bezier)
        __deref_outro_xcount((bType == MilCoreSeg::TypeLine) ? 1 : 3) const MilPoint2F *&pt
            // Line endpoint or curve 3 last points
        ) const;

    virtual bool SetToNextSegment() const;

    virtual __outro_ecount(1) const MilPoint2F &GetCurrentSegmentStart() const
    {
        ASSERT_COMPACT_VALID;
        Assert(m_uCurrentSegment < 4);
        return m_pt[m_uCurrentSegment];
    }

    virtual __outro_ecount(1) const MilPoint2F &GetStartPoint() const
    {
        ASSERT_COMPACT_VALID;
        return m_pt[0];
    }

    virtual __outro_ecount(1) const MilPoint2F &GetEndPoint() const
    {
        ASSERT_COMPACT_VALID;
        return m_pt[0];
    }
    
    virtual bool SetToLastSegment() const
    {
        m_uCurrentSegment = 3;
        return true;
    }
    
    virtual bool SetToPreviousSegment() const;

    // Other methods
    void Set(
        __in_ecount(1) const MilPointAndSizeF &rect
        )  // A rectangle 
    {
        Assert(!IsRectEmptyOrInvalid(&rect));

        // Note: This method does no other validation for MilPointAndSizeF's 
        // outside of the LTRB range.        
        CMilRectF rectRB(
            rect.X,
            rect.Y,
            rect.Width,
            rect.Height,
            XYWH_Parameters
            );

        Set(rectRB);
    }

    void Set(
        __in_ecount(1) const MilRectF &rect
        ); // A rectangle        

    void Set(
        __in_ecount(1) const CParallelogramFigure &other,
            // Another parallelogram
        __in_ecount_opt(1) const CBaseMatrix *pMatrix
            // Transformation (optional)
        );

    void GetBounds(
        __out_ecount(1) MilRectF &rect
        ) const; // The bounds

    void Transform(
        __in_ecount_opt(1) const CBaseMatrix *pMatrix
        );  // Transformation matrix (NULL OK)

    bool Contains(
        __in_ecount(1) const CParallelogramFigure &other,
        float rTolerance
        ) const;

protected:

    bool InternalIsAxisAlignedRectangle() const
    {
        ASSERT_COMPACT_VALID;

        // Check that the vertex at m_pt[0] has one horizontal and one vertical edge.
        // Since this is a parallelogram, all other edges comply with that.
        return ((m_pt[0].X == m_pt[3].X)  &&  (m_pt[0].Y == m_pt[1].Y) 
            ||
                (m_pt[0].Y == m_pt[3].Y)  &&  (m_pt[0].X == m_pt[1].X));
    }

protected:
    // Data
    CMilPoint2F      m_pt[4];
    mutable UINT    m_uCurrentSegment;
};
//+-----------------------------------------------------------------------------
//
//  Class:
//      CParallelogram
//
//  Synopsis:
//      A compact representation of a parallelogram shape
//
//------------------------------------------------------------------------------
class CParallelogram : public CCompactShape
{
    // Constructor/destructor
public:
    CParallelogram()
    {
    }

    virtual ~CParallelogram()
    {
    }

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CParallelogram));

    // IShapeData overides
    virtual __outro_ecount(1) const IFigureData &GetFigure(IN UINT index) const
    {
        Assert(index == 0);
        return m_figure;
    }

    virtual bool IsAxisAlignedRectangle() const
    {
        return InternalIsAxisAlignedRectangle();
    }

    virtual bool IsARegion() const
    {
        return InternalIsAxisAlignedRectangle();
    }

    virtual VOID GetRectangleCorners(
        __out_ecount(2)  MilPoint2F *pCorners       // 2 Rectangle's corners
        ) const
    {
        m_figure.GetRectangleCorners(pCorners);
    }

    virtual HRESULT GetTightBounds(__out_ecount(1) CMilRectF &rect) const
    {
        m_figure.GetBounds(rect);
        RRETURN(S_OK);        
    }

    // Other methods

    void Set(
        __in_ecount(1) const MilRectF &rect
        ) // A rectangle
    {
        m_figure.Set(rect);
    }

    void Set(
        __in_ecount(1) const CParallelogram &other,
            // Another parallelogram
        __in_ecount_opt(1) const CBaseMatrix *pMatrix
            // Transformation (optional)
        )
    {
        m_figure.Set(other.m_figure, pMatrix);
    }
       
    void Transform(
        __in_ecount_opt(1) const CBaseMatrix *pMatrix
        )  // Transformation matrix (NULL OK)
    {
        m_figure.Transform(pMatrix);
    }

    bool Contains(
        __in_ecount(1) const CParallelogram &other,
        float rTolerance
        ) const
    {
        return m_figure.Contains(
            other.m_figure,
            rTolerance
            );
    }

    VOID GetParallelogramVertices(
        __out_ecount(4)  MilPoint2F *pVertices // 4 Parallelogram Vertices
        ) const
    {
        m_figure.GetParallelogramVertices(pVertices);
    }


protected:

    bool InternalIsAxisAlignedRectangle() const
    {
         return m_figure.IsAxisAlignedRectangle();
    }

    virtual bool GetCachedBoundsCore(
        __out_ecount(1) MilRectF &rect
        ) const   // The bounds
    {
        m_figure.GetBounds(rect);
        return true;
    }
    
protected:
    // Data
    CParallelogramFigure   m_figure;
};
//+-----------------------------------------------------------------------------
//
//  Class:
//      CLineFigure
//
//  Synopsis:
//      A compact representation of a line segment as a figure
//
//------------------------------------------------------------------------------
class CLineFigure : public CCompactFigure
{
    // Constructor/destructor

public:
    CLineFigure()
    {
    }

    virtual ~CLineFigure()
    {
    }

    // IFigureData overrides
    virtual HRESULT GetCountsEstimate(
        __out_ecount(1) UINT &cSegments,    // A bound on the number of segments 
        __out_ecount(1) UINT &cPoints       // A bound on the number of points
        ) const
    {
        cSegments = 1;
        cPoints = 2;
        return S_OK;
    }

    virtual bool IsClosed() const
    {
        return false;
    }

    virtual bool IsFillable() const
    {
        return false;
    }

    virtual bool IsAParallelogram() const
    {
        return false;
    }

    virtual bool IsAxisAlignedRectangle() const
    {
        return false;
    }

    virtual VOID GetAsRectangle(
        __out_ecount(1) MilRectF &rect
        ) const
    {
        RIP("GetAsRectangle called on CLine");
    }

    virtual VOID GetAsWellOrderedRectangle(
        __out_ecount(1) MilRectF &rect
        ) const
    {
        RIP("GetAsWellOrderedRectangle called on CLine");
    }

    virtual VOID GetParallelogramVertices(
        __out_ecount(4)  MilPoint2F            *pVertices,    // 4 Parallelogram's vertices
        __in_ecount_opt(1)  const CMILMatrix  *pMatrix=NULL  // Transformation (optional)
        ) const
    {
        RIP("GetParallelogramVertices called on CLine");
    }
    
    virtual VOID GetRectangleCorners(
        __out_ecount(2)  MilPoint2F *pCorners       // 2 Rectangle's corners
        ) const
    {
        RIP("GetRectangleCorners called on CLine");
    }

    virtual bool SetToFirstSegment() const
    {
        return true;
    }
    
    virtual bool GetCurrentSegment(     // Returns true if this is the last segment
        OUT BYTE            &bType,     // Segment type (line or Bezier)
        OUT const MilPoint2F *&pt        // Line endpoint or curve 3 last points
        ) const
    {
        ASSERT_COMPACT_VALID;

        pt = &m_pt[END_POINT];
        bType = MilCoreSeg::TypeLine;
        return false;           // Stops do not apply
    }

    virtual bool SetToNextSegment() const
    {
        return false;
    }

    virtual __outro_ecount(1) const MilPoint2F &GetCurrentSegmentStart() const
    {
        ASSERT_COMPACT_VALID;
        return m_pt[START_POINT];
    }

    virtual __outro_ecount(1) const MilPoint2F &GetStartPoint() const
    {
        ASSERT_COMPACT_VALID;
        return m_pt[START_POINT];
    }

    virtual __outro_ecount(1) const MilPoint2F &GetEndPoint() const
    {
        ASSERT_COMPACT_VALID;
        return m_pt[END_POINT];
    }
    
    virtual bool SetToLastSegment() const
    {
        return true;
    }
    
    virtual bool SetToPreviousSegment() const
    {
        return false;
    }

    // Other methods
    void Set(
        FLOAT X0,   FLOAT Y0,   // The startpoint
        FLOAT X1,   FLOAT Y1
        )   // The endpoint
    {
        m_pt[START_POINT].X = X0;
        m_pt[START_POINT].Y = Y0;
        m_pt[END_POINT].X = X1;
        m_pt[END_POINT].Y = Y1;

        SET_COMPACT_VALID(true);
    }

    void GetBounds(
        __out_ecount(1) MilRectF &rect
        ) const; // The bounds

    void Transform(
        __in_ecount_opt(1) const CMILMatrix *pMatrix
        );  // Transformation matrix (NULL OK)

protected:
    // Data   
    MilPoint2F           m_pt[2];

    static const UINT  START_POINT = 0;
    static const UINT  END_POINT   = 1;
};
//+-----------------------------------------------------------------------------
//
//  Class:
//      CLine
//
//  Synopsis:
//      A compact representation of a line segment as a shape
//
//------------------------------------------------------------------------------
class CLine : public CCompactShape
{
    // Constructor/destructor

public:
    CLine()
    {
    }

    void Set(
        FLOAT X0,   FLOAT Y0,   // The startpoint
        FLOAT X1,   FLOAT Y1)   // The endpoint
    {
        m_figure.Set(X0, Y0, X1, Y1);
    }

    virtual ~CLine()
    {
    }

    // IShapeData overrides
    virtual __outro_ecount(1) const IFigureData &GetFigure(IN UINT index) const
    {
        Assert(index == 0);
        return m_figure;
    }

    virtual bool IsAxisAlignedRectangle() const
    {
        return false;
    }

    virtual HRESULT GetTightBounds(__out_ecount(1) CMilRectF &rect) const 
    {
        m_figure.GetBounds(rect);
        RRETURN(S_OK);        
    }    

    // Other methods
    void Transform(
        __in_ecount_opt(1) const CMILMatrix *pMatrix
        )  // Transformation matrix (NULL OK)
    {
        m_figure.Transform(pMatrix);
    }

protected:

    virtual bool GetCachedBoundsCore(
        __out_ecount(1) MilRectF &rect
        ) const   // The bounds
    {
        m_figure.GetBounds(rect);
        return true;
    }    

protected:
    // Data
    CLineFigure     m_figure;
};


