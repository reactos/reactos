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
//      Definition of CShape.
//
//  $Notes:
//      IF YOU WRITE ANY CODE THAT MAY CHANGE THE GEOMETRY OF THE SHAPE (E.G.
//      ADD FIGURES, CHANGE POINTS COORDINATES), BE SURE TO HAVE THAT CODE CALL
//      INVALIDATECACHE!
//
//  $ENDTAG
//
//  Classes:
//      CShape CCompoundShapeNoRef.
//
//------------------------------------------------------------------------------

MtExtern(CShape);

// Shape cache validity bits
const WORD SHAPE_BOX_VALID          = 0x0001;
const WORD SHAPE_HAS_CORNERS_VALID  = 0x0002;

// Enum tokens used to clarify whether ellipse is specified as center/radii
// or origin/width/height.
typedef enum CR { CR_Parameters } CR;
typedef enum OWH { OWH_Parameters } OWH;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CShape
//
//  Synopsis:
//      CShapeBase which can represent any abitrary geometry. This CShapeBase
//      implementation is more powerful, and more expensive, than the
//      CCompactShape representations.
//
//------------------------------------------------------------------------------
class CShape : public CShapeBase
{
public:
    // Constructor/destructor
    CShape()
        :   m_eFillMode(MilFillMode::Winding),
            m_wCacheState(SHAPE_BOX_VALID),
            m_fFillState(true)    
    {
        m_cachedBounds.left = m_cachedBounds.right = m_cachedBounds.top = m_cachedBounds.bottom = 0;
    }

    virtual ~CShape();
    
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CShape));

    // IShapeBuilder overrides
    virtual HRESULT AddNewFigure(
        __deref_out_ecount(1) IFigureBuilder *&pFigure);   // The newly added figure
    
    virtual HRESULT AddRect(
        __in_ecount(1) const MilRectF &rect,             // The rectangle
        __in_ecount_opt(1) const CMILMatrix *pMatrix=NULL); // Optional: Transformation matrix (NULL OK)

    // IShapeData overrides
    virtual bool HasGaps() const;

    virtual bool HasHollows() const;
    
    virtual bool IsEmpty() const;

    virtual UINT GetFigureCount() const
    {
        return m_rgFigures.GetCount();
    }

    virtual __outro_ecount(1) const IFigureData &GetFigure(IN UINT index) const
    {
        Assert(index < GetFigureCount());
        Assert(m_rgFigures[index]); // No null figures allowed in the shape
        return *m_rgFigures[index];
    }

    virtual MilFillMode::Enum GetFillMode() const
    {
        return m_eFillMode;
    }

    virtual bool IsAxisAlignedRectangle() const
    {
        return (1 == GetFigureCount() && GetFigure(0).IsAxisAlignedRectangle());
    }

    // Other methods
    HRESULT Copy(
        __in_ecount(1) const CShape &other);     // The shape to copy

    void Transform(
        __in_ecount_opt(1) const CBaseMatrix *pMatrix);  // Transformation matrix (NULL OK)

    void Reset(IN bool shrink=TRUE);

    __out_ecount(1) CFigureData &GetModifyFigure(
        IN UINT index);          // Index

    __outro_ecount(1) const CFigureData &GetFigureData(IN int index) const
    {
        return *m_rgFigures[index];
    }

    HRESULT AddFigure(
        __deref_out_ecount(1) CFigureData *&pOutFigure);   // The newly added figure

    HRESULT AddAndTakeOwnership(__inout_ecount(1) CFigureData *pFigure);

    void InvalidateCache();

    void SetFillMode(MilFillMode::Enum eMode)
    {
        m_eFillMode = eMode;
    }

    bool GetFillState() const
    {
        return m_fFillState;
    }

    void SetFillState(IN BOOL fValue)
    {
        m_fFillState = (TRUE == fValue);
    }

    HRESULT ConstructFromGpPath(
        IN MilFillMode::Enum eMode,
            // FillMode
        IN INT count,
            // Number of points & types
        __in_ecount(count) const MilPoint2F  *pPoints,
            // Path points
        __in_ecount(count) const BYTE *pTypes);
            // Path types
    
    HRESULT AddFigureFromRawData(
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
  
    HRESULT AddCopyOf(
        __in_ecount(1) const CFigure &figure);  // figure to copy

    HRESULT AddLine(
        REAL x1, REAL y1,
        REAL x2, REAL y2);   // In: Line's end point

    HRESULT AddLines(
        __in_ecount(count) const MilPoint2F *rgPoints,
            // In: Array of points
        INT count);
            // In: Number of points

    HRESULT AddBezier(
        REAL x1, REAL y1,   // In: First Bezier point
        REAL x2, REAL y2,   // In: Second Bezier point
        REAL x3, REAL y3,   // In: Third Bezier point
        REAL x4, REAL y4);  // In: Fourth Bezier point

    HRESULT AddBeziers(
        __in_ecount(count) const MilPoint2F *rgPoints,
            // In: Array of points
        INT count);
            // In: Number of points

    HRESULT AddRectangle(
        REAL x, REAL y,  // In: The rectangle's corner
        REAL width,      // In: Defining rectangle's width
        REAL height);    // In: Defining rectangle's height

    HRESULT AddRectangles(
        __in_ecount(count) const MilPointAndSizeF *rgRects, // In: Array of rectangles
        INT count);             // In: Number of points

    HRESULT AddEllipse(     // The following parameters refer to the rectangular bounds of the ellipse:
        REAL x, REAL y,     // The upper left hand corner.
        REAL width,         // Rectangle width
        REAL height,        // Rectangle height
        OWH owhParameters
        );

    HRESULT AddEllipse(
        REAL centerX, REAL centerY, // The center of the ellipse
        REAL radiusX,               // The X-radius of the ellipse
        REAL radiusY,               // The Y-radius of the ellipse
        CR crParameters
        );

    HRESULT AddRoundedRectangle(
        __in_ecount(1) const MilPointAndSizeF &rect,
            // The bounding rectangle
        IN REAL radiusX,
            // The X-radius of the corner (elliptical arc)
        IN REAL radiusY
            // The Y-radius of the corner (elliptical arc)
        );

    HRESULT AddBeveledRectangle(
        __in_ecount(1) const MilPointAndSizeF &rect,
            // The bounding rectangle
        IN REAL rBevelOffset
            // The bevel offset
        );

    HRESULT AddPolygon(
        __in_ecount(1) const MilPoint2F *rgPoints,
            // In: Array of points
        INT count);
            // In: Number of points

    HRESULT AddShape(
        __in_ecount(1) const CShape &shape);   // The shape to add

    HRESULT AddShapeData(
        __in_ecount(1) const IShapeData    &shape,          // The shape to add
        __in_ecount_opt(1) const CMILMatrix *pMatrix=NULL); // Transformation to apply to the input

#ifdef DBG
    void Dump() const;
#endif

public:
    // Statics
    __outro_ecount(1) static const CShape *EmptyShape() { return &s_emptyShape; }
    
    // Private methods
private:

    void Abort();

    // Special case for Combine
    HRESULT IntersectAxisAlignedRectangles(
        __in_ecount(1) const CShape *pFirst,
            // IN: First operand
        __in_ecount(1) const CShape *pSecond,
            // IN: Second operand
        __in_ecount_opt(1) const CMILMatrix *pFirstTransform,
            // IN: Transform for the first shape (NULL OK)
        __in_ecount_opt(1) const CMILMatrix *pSecondTransform,
            // IN: Transform for the second shape (NULL OK)
        __out_ecount(1) BOOL *pfPerformedIntersect
            // OUT: returns whether the intersect happened
        );

protected:
    void operator = (const CShape &)
    {
    }

    virtual bool GetCachedBoundsCore(
        __out_ecount(1) MilRectF &rect) const;    // The cached bounds, set only if valid

    virtual void SetCachedBounds(
        __in_ecount(1) MilRectF const &rect) const;  // Bounding box to cache    

protected:
    
    // Data
    DynArrayIA<CFigureData*, 2> m_rgFigures;     // Figures
    MilFillMode::Enum                 m_eFillMode;     // Fill mode
    bool                        m_fFillState;    // Added figures will fill if true

    // Cached info

    // A cached CFigure to avoid an allocation in common cases.
    // In a given CShape, this figure may or may not be used.
    // If it is, it is used for the first figure.
    CFigureData             m_oCachedFigure;

    mutable MilRectF     m_cachedBounds;  // Bounding box
    mutable WORD            m_wCacheState;   // The cache state bits

private:
    // Static data
    static const CShape s_emptyShape;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CCompoundShapeNoRef
//
//  Synopsis:
//      Maintains an array of IShapeData pointers as a fast alternative to
//      combining shapes.
//
//      READ ME BEFORE USING!!!!
//
//      This class does not copy the shapedatas for performance reasons. Only
//      use it in narrowly scoped scenarios to concatenate multiple IShapeDatas
//      into 1.
//
//      This class doesn't support to following:
//        1.  Overlapping sub-shapes.  All shapes added must be disjoint.
//
//      Also GetFigure has not been optimized.  Currently it has a linear
//      running time.
//
//      The class could be optimized even further using preallocated memory so
//      the dynamic array wouldn't have to alloc/free.
//
//      We allow the children shapes to have different fill modes with the
//      caveat that the fill mode will be ignored.
//
//------------------------------------------------------------------------------
class CCompoundShapeNoRef :
    public IShapeData
{
public:
    CCompoundShapeNoRef()
    {
#if DBG
        m_fDbgHaveShapesBeenWalkedToPrecomputeProperties = false;
#endif
        m_rcCachedBounds = CMilRectF::sc_rcEmpty;
        m_fCachedBoundsSet = false;
        m_eFillMode = MilFillMode::Winding;
    };

    //
    // IShapeData virtual methods
    //
    virtual bool HasGaps() const
    {
#if DBG
        Assert(m_fDbgHaveShapesBeenWalkedToPrecomputeProperties);
#endif

        return m_fHasGaps;
    }

    virtual bool HasHollows() const
    {
#if DBG
        Assert(m_fDbgHaveShapesBeenWalkedToPrecomputeProperties);
#endif

        return m_fHasHollows;
    }

    virtual bool IsEmpty() const
    {
#if DBG
        Assert(m_fDbgHaveShapesBeenWalkedToPrecomputeProperties);
#endif

        return m_fIsEmpty;
    }

    virtual UINT GetFigureCount() const
    {
#if DBG
        Assert(m_fDbgHaveShapesBeenWalkedToPrecomputeProperties);
#endif

        return m_uFigureCount;
    }

    virtual __outro_ecount(1) const IFigureData & GetFigure(UINT index) const;

    virtual MilFillMode::Enum GetFillMode() const
    {
        return m_eFillMode;
    }

    void SetFillMode(MilFillMode::Enum eMode)
    {
        m_eFillMode = eMode;
    }

    virtual bool IsAxisAlignedRectangle() const
    {
#if DBG
        Assert(m_fDbgHaveShapesBeenWalkedToPrecomputeProperties);
#endif

        return m_fIsAxisAlignedRectangle;
    }

    bool IsARegion() const
    {
#if DBG
        Assert(m_fDbgHaveShapesBeenWalkedToPrecomputeProperties);
#endif

        return m_fIsARegion;
    }

    virtual bool GetCachedBoundsCore(__out_ecount(1) MilRectF &rcBoundsShapeSpace) const
    {
        Assert(m_fCachedBoundsSet == true);

        rcBoundsShapeSpace = m_rcCachedBounds;

        return true;
    }

    virtual void SetCachedBounds(__in_ecount(1) const MilRectF &rect) const
    {
        RIP("Shouldn't ever have to set cached bounds on the CCompoundShapeNoRef class");
    }

    //
    // CCompoundShapeNoRef methods
    //
    HRESULT SetShapeDatasNoRef(
        __in_ecount(uNumShapes) IShapeData **rgpNewShapes,
        UINT uNumShapes
        );

private:
    //
    // We pick 4 elements here for the Initial allocation because the only
    // place CCompoundShapeNoRef is currently used is for the dwm scenario,
    // which has 4 shapes.
    //
    DynArrayIA<IShapeData *, 4> m_rgpShapeDatas;
    MilRectF m_rcCachedBounds;


    UINT m_uFigureCount;
    bool m_fHasGaps;
    bool m_fHasHollows;
    bool m_fIsEmpty;
    bool m_fIsAxisAlignedRectangle;
    bool m_fIsARegion;

    bool m_fCachedBoundsSet;

    MilFillMode::Enum m_eFillMode;     // Fill mode

#if DBG
    bool m_fDbgHaveShapesBeenWalkedToPrecomputeProperties;
#endif

private:

    HRESULT WalkShapesAndPrecomputeProperties();

    UINT GetFigureCountInternal() const;

    HRESULT GetFillBoundsInternal(
        __out_ecount(1) MilRectF *prcBoundsShapeSpace
        ) const;

    bool HasGapsInternal() const;

    bool HasHollowsInternal() const;

    bool IsEmptyInternal() const;

    bool IsAxisAlignedRectangleInternal() const;

    bool IsARegionInternal() const;

    MIL_FORCEINLINE MilFillMode::Enum GetDefaultFillMode() const
    {
        return MilFillMode::Winding;
    }
};


