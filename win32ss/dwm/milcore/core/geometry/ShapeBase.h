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
//      Definition of CShapeBase.
//
//  $Notes:
//      Despite its name, CShapeBase is not a base class for any derived
//      classes.  It could be described as a shape processor.  It holds a
//      reference to geometric data via the IShapeDAta interface, and it
//      processes that data in a way that is indpenedent of the data
//      representation.
//
//  $ENDTAG
//
//  Classes:
//      CShapeBase.
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Class:
//      CShapeBase
//
//  Synopsis:
//      The methods for processing shapes
//
//------------------------------------------------------------------------------
class CShapeBase 
{
public:
    // Constructor/destructor
    CShapeBase()
    {
    }

    virtual ~CShapeBase()
    {        
    }

    //
    // Abstract virtual methods which must be 
    // implemented by subclasses
    //

    virtual bool HasGaps() const = 0;

    virtual bool HasHollows() const = 0;
    
    virtual bool IsEmpty() const = 0;

    virtual UINT GetFigureCount() const = 0;

    virtual __outro_ecount(1) const IFigureData &GetFigure(IN UINT index) const = 0;

    virtual MilFillMode::Enum GetFillMode() const = 0;

    virtual bool IsAxisAlignedRectangle() const = 0;

protected:
    
    virtual bool GetCachedBoundsCore(
        __out_ecount(1) MilRectF &rect) const = 0;    // The cached bounds, set only if valid

    virtual void SetCachedBounds(
        __in_ecount(1) const MilRectF &rect) const = 0;  // Bounding box to cache           

    //
    // Virtual methods which may be overridden by subclasses
    //        

public:    

    virtual bool IsARegion() const
    {
        // Implementation not mandatory
        return false;
    }    

    virtual HRESULT GetTightBounds(
        __out_ecount(1) CMilRectF &rect
        ) const
    {
        // Default implementation defers to general (and less optimal)
        // CShapeBase implementation        
        INLINED_RRETURN(GetTightBounds(rect, NULL, NULL));
    }

    virtual HRESULT GetTightBounds(
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
        __in bool fSkipHollows=true) const;
            // If true, skip non-fillable figures when computing fill bounds (optional)       

    HRESULT GetTightBoundsNoBadNumber(
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
        __in bool fSkipHollows=true
            // Should hollows be skipped? (optional)
        ) const;

    HRESULT GetTightBoundsNoBadNumber(
        __out_ecount(1) CMilRectF &rect
        ) const
    {
        INLINED_RRETURN(GetTightBoundsNoBadNumber(rect, NULL, NULL));
    }
    
    virtual HRESULT WidenToShape(
        __in_ecount(1) const            CPlainPen &pen,
            // The pen
        double rTolerance,
            // Flattening tolerance
        bool fRelative,
            // True if the tolerance is relative       
        __inout_ecount(1) CShape &widened,
            // The widened shape, populated here
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Render transform (NULL OK)
        __in_ecount_opt(1) const CMILSurfaceRect *prcClip = NULL
            // Viewable region (NULL OK)
        ) const;

    //
    // Non-virtual general geometry functionality 
    //

    HRESULT ConvertToGpPath(
        __out_ecount(1) DynArray<MilPoint2F> &rgPoints,       
            // Path points
        __out_ecount(1) DynArray<BYTE>      &rgTypes,       
            // Path types
        IN  bool                fStroking=false
            // Stroking if true, filling otherwise (optional)
        ) const;

    HRESULT FlattenToShape(
        IN double rTolerance,           
            // Flattening tolerance
        IN bool fRelative,               
            // True if the tolerance is relative       
        __inout_ecount(1) IShapeBuilder &flattened,    
            // The flattened shape
        __in_ecount_opt(1) const CMILMatrix *pMatrix=NULL
            // Optional: Transformation (NULL OK)
        ) const;

    HRESULT WidenToSink(
        __in_ecount(1) const CPlainPen &pen,
            // The pen
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Render transform (NULL OK)
        double rTolerance,
            // Approximation tolerance - absolute
        __inout_ecount(1) CWideningSink &sink,
            // The widening sink
        __in_ecount_opt(1) const CMILSurfaceRect *prcClip = NULL,
            // Viewable region (NULL OK)
        __out_ecount_opt(1) bool *pfPenEmpty = NULL
            // If true, we earlied out because the pen is either empty or very
            // close to it (NULL OK)
        ) const;

    HRESULT SetupFillTessellator(
        __in_ecount_opt(1) const CBaseMatrix *pMatrix,
            // Transformation matrix (NULL OK)
        __in_ecount_opt(1) CBufferDispenser *pBufferDispenser,
            // Fast allocator (NULL OK)
        __deref_out_ecount(1) CFillTessellator **ppTessellator
            // Shape/context optimized Tessellator
        ) const;

    HRESULT Outline(
        __inout_ecount(1) IShapeBuilder &result,
            // The outline shape, populated here
        __in double rTolerance=DEFAULT_FLATTENING_TOLERANCE,
            // Flattening tolerance
        __in bool fRelative=false,
            // True if the tolerance is relative       
        __in_ecount_opt(1) const CMILMatrix *pMatrix=NULL,
            // Transformation matrix (NULL OK)
        __in bool fRetrieveCurves=true) const;
            // Retrieve curves in the result if true

    static HRESULT CShapeBase::ClipWithParallelogram(
        __in_ecount(1) const IShapeData *pShape,
            // Shape to be clipped
        __in_ecount(1) const CParallelogram *pClipParallelogram,
            // Parallelogram to clip to
        __inout_ecount(1) IShapeBuilder *pResult,
            // The recipient of the result
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pShapeTransform,
            // Transform for the first shape (Optional, NULL OK)
        __in double rTolerance=DEFAULT_FLATTENING_TOLERANCE,
            // Flattening tolerance
        __in bool fRelative=false);
            // True if the tolerance is relative       

    static HRESULT ClipWithRect(
        __in_ecount(1) const IShapeData *pShape,
            // Shape to be clipped
        __in_ecount(1) const CRectF<CoordinateSpace::Device> *prcClip,
            // Rect to clip to
        __inout_ecount(1) IShapeBuilder *pResult,
            // The recipient of the result
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pShapeTransform,
            // Transform for pShape (Optional, NULL OK)
        __in double rTolerance=DEFAULT_FLATTENING_TOLERANCE,
            // Flattening tolerance
        __in bool fRelative=false);
            // True if the tolerance is relative       

    static HRESULT Combine(
        __in_ecount(1) const IShapeData *pFirst,
            // First operand
        __in_ecount(1) const IShapeData *pSecond,
            // Second operand
        __in MilCombineMode::Enum eOperation,
            // The operation
        __in bool fRetrieveCurves,
            // Retrieve curves in the result if true
        __inout_ecount(1) IShapeBuilder *pResult,
            // The recipient of the result
        __in_ecount_opt(1) const CMILMatrix *pFirstTransform=NULL,
            // Transform for the first shape (Optional, NULL OK)
        __in_ecount_opt(1) const CMILMatrix *pSecondTransform=NULL,
            // Transform for the second shape (Optional, NULL OK)
        __in double rTolerance=DEFAULT_FLATTENING_TOLERANCE,
            // Flattening tolerance
        __in bool fRelative=false);
            // True if the tolerance is relative       

    HRESULT HitTestFill(
        __in_ecount(1) const MilPoint2F  &ptHit,
            // The point
        double rThreshold,
            // Distance considered near
        IN bool fRelative,
            // True if the threshold is relative       
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Transformation to apply to the shape (NULL OK)
        __out_ecount(1) BOOL &fHit,
            // = TRUE if the point is in or near the shape
        __out_ecount(1) BOOL &fIsNear
            // = TRUE if the point is close to the boundary
        ) const;

    HRESULT HitTestFiguresFill(
        __inout_ecount(1) CHitTest &tester  // A hit tester
        ) const;

    HRESULT HitTestStroke(
        __in_ecount(1) const CPlainPen &pen,
            // The pen with which we stroke
        __in_ecount(1) const MilPoint2F &ptHit,
            // The point
        IN double rThreshold,
            // Distance considered near
        IN bool fRelative,
            // True if the threshold is relative       
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Transformation to apply to the shape (NULL OK)
        __out_ecount(1) BOOL &fHit,
            // = TRUE if the point is in or near the shape
        __out_ecount(1) BOOL &fIsNear 
            // = TRUE if the point is close to the boundary
        ) const;

    HRESULT GetRelativeTightBoundsNoBadNumber(
        __out_ecount(1) CMilRectF &rect,           // The bounds of this shape
        __in_ecount_opt(1) const CPlainPen  *pPen,    // The pen
        __in_ecount_opt(1) const CMILMatrix *pMatrix, // Transformation
        __in double rRelativeTolerance = 0.001  // Error tolerance, relative to the loose bounds.  Default is .1%.
        ) const
    {
        INLINED_RRETURN(GetTightBoundsNoBadNumber(rect, pPen, pMatrix, rRelativeTolerance, true));
    }

    HRESULT GetRelativeTightBounds(
        __out_ecount(1) CMilRectF &rect,           // The bounds of this shape
        __in_ecount_opt(1) const CPlainPen  *pPen,    // The pen
        __in_ecount_opt(1) const CMILMatrix *pMatrix, // Transformation
        __in double rRelativeTolerance = 0.001  // Error tolerance, relative to the loose bounds.  Default is .1%.
        ) const
    {
        INLINED_RRETURN(GetTightBounds(rect, pPen, pMatrix, rRelativeTolerance, true));
    }

    HRESULT GetLooseBounds(
        __out_ecount(1) CMilRectF &rect,              // The bounds of this shape
        __in_ecount_opt(1) const CPlainPen *pPen=NULL,   // The pen (optional, NULL OK)
        __in_ecount_opt(1) const CBaseMatrix *pMatrix=NULL// Transformation (optional, NULL OK)
        ) const; 

    HRESULT UpdateBounds(
        __inout_ecount(1) CBounds &bounds,
            // Bounds, updated here
        __in bool fFillOnly,
            // Skip non-fillable figures if true
        __in_ecount_opt(1) const CMILMatrix *pMatrix
            // Transformation (NULL OK)
        ) const;

    HRESULT GetCachedBounds(
        __out_ecount(1) CMilRectF &rect) const;     // The cached bounds

    // Populate a scanner
    HRESULT Populate(
        __in_ecount(1) IPopulationSink *pPopSink,
            // The receiving sink
        __in_ecount_opt(1) const CBaseMatrix *pMatrix=NULL) const;
            // Transformation matrix (NULL OK)

    HRESULT GetRelation(
        __in_ecount(1) const IShapeData  &data,      // The other shape
        __in double rTolerance, // Flattening tolerance
        __in bool fRelative,  // True if the tolerance is relative       
        __out_ecount(1) MilPathsRelation::Enum &eResult    // The operation
        ) const;

    HRESULT GetArea(
        IN double rTolerance,
            // Flattening tolerance
        IN bool fRelative,
            // True if the tolerance is relative       
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Transformation to apply to the shape (NULL OK)
        __out_ecount(1) double &result
            // The area, not set if error encountered
        ) const;

#ifdef DBG
    void Dump() const;
#endif

    // Private methods
private:

    HRESULT GetStrokeBounds(
        __out_ecount(1) CMilRectF &rect,
            // Out: The bounds of this shape
        __out_ecount_opt(1) bool *pfNotCalculated,
            // Out: The calculation short-circuited due, for instance, to a small pen.
            // (NULL OK)
        __in_ecount(1) const CPlainPen &pen,
            // In: The pen
        __in_ecount_opt(1) const CMILMatrix *pMatrix=NULL,
            // In: Transformation (optional, NULL OK)
        double rTolerance=0
            // In: Error tolerance (optional)
        ) const;

    HRESULT GetFillBounds(
        __out_ecount(1) CMilRectF &rect,
            // The bounds of this shape
        __in bool fFillOnly,
            // Skip non-fillable figures if true
        __in_ecount_opt(1) const CMILMatrix *pMatrix=NULL
            // Transformation (NULL OK)
        ) const;

    static HRESULT IntersectAxisAlignedRectangles(
        __in_ecount(1) const CShapeBase &first,
            // First operand
        __in_ecount(1) const CShapeBase &second,
            // Second operand
        __inout_ecount(1) IShapeBuilder *pResult,
            // The recipient of the result
        __in_ecount_opt(1) const CMILMatrix *pFirstTransform,
            // Transform for the first shape (NULL OK)
        __in_ecount_opt(1) const CMILMatrix *pSecondTransform,
            // Transform for the second shape (NULL OK)
        __out_ecount(1) BOOL *pfPerformedIntersect
            // =true if the intersect happened
        );

    HRESULT GetAbsoluteTolerance(
        __in double rTolerance,
            // The tolerance
        __in bool fRelative,
            // =true if the tolerance is relative
        __in_ecount_opt(1) const CPlainPen  *pPen,
            // A pen that may be stroking this shape
        __in_ecount_opt(1) const CBaseMatrix *pMatrix,
            // Transformation matrix
        __out_ecount(1) double &rAbsolute
            // absolute tolerance
        ) const;

    BOOL GetPointOnShape(__out_ecount(1) MilPoint2F *point) const;
};



