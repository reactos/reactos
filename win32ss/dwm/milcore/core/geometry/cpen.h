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
//      Contains the definition of the CPlainPen.
//
//  $Notes:
//      1/7/2002 Michka
//          Created it, merging DpPen and GpPen into the new CPlainPen class,
//          removing all reference to brush
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#ifndef _CPEN_H
#define _CPEN_H

extern const BYTE PENPLAIN_HAS_START_SHAPE;
extern const BYTE PENPLAIN_HAS_END_SHAPE;

class CPlainPen;

////////////////////////////////////////////////////////////////////////////
// Definition of CPenGeometry
// This class captures geometry the stroke emitted by a simple pen:
// No dashes, compound or line shapes

class CPenGeometry
{
public:
    CPenGeometry()
    :m_rWidth(1),
    m_rHeight(1),
    m_rAngle(0),
    m_eStartCap(MilPenCap::Flat),
    m_eEndCap(MilPenCap::Flat),
    m_eDashCap(MilPenCap::Flat),
    m_eJoin(MilLineJoin::Miter),
    m_rMiterLimit(10)
    {
    }

    CPenGeometry(
        __in_ecount(1) const CPenGeometry &other)
    :m_rWidth(other.m_rWidth),
    m_rHeight(other.m_rHeight),
    m_rAngle(other.m_rAngle),
    m_eStartCap(other.m_eStartCap),
    m_eEndCap(other.m_eEndCap),
    m_eDashCap(MilPenCap::Flat),
    m_eJoin(other.m_eJoin),
    m_rMiterLimit(other.m_rMiterLimit)
    {
    }

    bool IsCircular() const
    {
        return m_rWidth == m_rHeight;
    }

    void Set(
        REAL width,   // In: Pen ellipse width
        REAL height,  // In: Pen ellipse height
        REAL angle);  // In: Angle in radians the ellipse is rotated

    REAL GetWidth() const
    {
        return m_rWidth;
    }
            
    void SetWidth(IN REAL rWidth)
    {
        m_rWidth = fabs(rWidth);
    }
            
    REAL GetHeight() const
    {
        return m_rHeight;
    }
            
    void SetHeight(IN REAL rHeight)
    {
        m_rHeight = fabs(rHeight);
    }

    REAL GetAngle() const
    {
        return m_rAngle;
    }
            
    void SetAngle(
        REAL val)
    {
        m_rAngle = val;
    }

    bool IsEmpty() const
    {
        return m_rWidth == 0  ||  m_rHeight == 0;
    }

    MilPenCap::Enum GetStartCap() const
    {
        return m_eStartCap;
    }

    void SetStartCap(MilPenCap::Enum eCap)
    {
        m_eStartCap = eCap;
    }

    MilPenCap::Enum GetEndCap() const
    {
        return m_eEndCap;
    }

    void SetEndCap(MilPenCap::Enum eCap)
    {
        m_eEndCap = eCap;
    }

    MilPenCap::Enum GetDashCap() const
    {
        return m_eDashCap;
    }

    void SetDashCap(MilPenCap::Enum eCap)
    {
        m_eDashCap = eCap;
    }

    MilLineJoin::Enum GetJoin() const
    {
        return m_eJoin;
    }

    void SetJoin(MilLineJoin::Enum eJoin)
    {
        m_eJoin = eJoin;
    }

    REAL GetMiterLimit() const
    {
        return m_rMiterLimit;
    }
            
    void SetMiterLimit(REAL val)
    {
        m_rMiterLimit = max(val, 1.0f);
    }

    REAL GetInflateFactor() const;

    REAL GetExtents() const
    {
        return GetInflateFactor() * max(m_rWidth, m_rHeight);
    }

protected:
    // Allow A line shape to override the corresponding line cap
    friend class CPlainPen;

    // Data
    REAL            m_rWidth;
    REAL            m_rHeight;
    REAL            m_rAngle;

    MilPenCap::Enum    m_eStartCap;
    MilPenCap::Enum    m_eEndCap;
    MilPenCap::Enum    m_eDashCap;
    MilLineJoin::Enum    m_eJoin;
    REAL           m_rMiterLimit;
};

MtExtern(CPlainPen);

////////////////////////////////////////////////////////////////////////////
// Definition of CPlainPen
// This class captures geometric properties of the stroke emitted by a pen:
// No concept of color or brush.
// The class design has hooks for compound lines, but the feature is not yet
// implemented in our widening code.  Until it is implemented, these hooks are
// hidden inside #ifdef COMPOUND_PEN_IMPLEMENTED.

class CPlainPen
{
public:
    CPlainPen();

    ~CPlainPen();

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CPlainPen));

    CPlainPen(__in_ecount(1) const CPlainPen &other);

    void Set(
        REAL width,   // In: Pen ellipse width
        REAL height,  // In: Pen ellipse height
        REAL angle)   // In: Angle in radians the ellipse is rotated
    {
        m_oGeom.Set(width, height, angle);
    }

    bool IsEmpty() const
    {
        return m_oGeom.IsEmpty();
    }

    BOOL IsCircular() const
    {
        return m_oGeom.IsCircular();
    }

    BOOL IsSimple() const
    {
        return ((m_eDashStyle == MilDashStyle::Solid)  &&
                (NULL == m_pStartShape) && (NULL == m_pEndShape));
    }

    BOOL IsCompound() const
    {
#ifdef COMPOUND_PEN_IMPLEMENTED
        return ((m_rgCompound.GetCount() > 0));
#else
        return false;
#endif
    }
    
    HRESULT Clone(
        __deref_out_ecount(1) CPlainPen *&pClone) const;
            // Out: The clone    
    
    HRESULT SetDashStyle(
        IN MilDashStyle::Enum style);     // Dash style, cannot be custom
    
    HRESULT GetDashArray(
        IN UINT count,
            // Output buffer size
        __out_ecount(count) /* _part(count, m_rgDashes.Count) */ REAL *dashes);
            // The arrray of dash starts/ends
    
    HRESULT SetDashArray(
        __in_ecount_opt(count) const REAL* dashes,
            // In: The arrray of dash starts/ends
        INT count);
            // In: Size of the array

#ifdef COMPOUND_PEN_IMPLEMENTED    
    HRESULT GetCompoundArray(
        IN UINT count,
            // Output buffer size
        __out_ecount(count) /* _part(count, m_rgCompound.Count) */ REAL *array);
            // The Output arrray

    HRESULT SetCompoundArray(
        __in_ecount(count) const REAL *array,
            // In: Compound array values to set
        INT count);
            // In: Number of entries in the array

    INT GetCompoundCount() const
    {
        return m_rgCompound.GetCount(); 
    }
#endif // COMPOUND_PEN_IMPLEMENTED
        
    HRESULT SetStartShape(MilLineShape::Enum eType)    // In: The type
    {
        return SetCannedLineShape(eType, m_pStartShape, m_eStartShapeType, m_oGeom.m_eStartCap);
    }

    MilLineShape::Enum GetStartShape()
    {
        return m_eStartShapeType;
    }
    
    HRESULT SetEndShape(MilLineShape::Enum eType)    // In: The type
    {
        return SetCannedLineShape(eType, m_pEndShape, m_eEndShapeType, m_oGeom.m_eEndCap);
    }
    
    MilLineShape::Enum GetEndShape()
    {
        return m_eEndShapeType;
    }
    
    HRESULT SetCustomStartShape(
        __in_ecount(1) const CShape &shape,
            // The line shape's geometry        
        IN FLOAT        rInset,
            // The amount the path will be trimmed
        IN FLOAT        rAnchorDistance,
            // The -Y coordinate of the anchor point 
        IN bool         fFill,
            // Fill the shape if true 
        IN bool         fStroke,
            // Stroke the shape if true
        __in_ecount_opt(1) CPlainPen    *pPen)
            // An overriding pen for strokes  (NULL OK)
    {
        return SetCustomLineShape(
            shape, 
            rInset, 
            rAnchorDistance, 
            fFill, 
            fStroke,
            pPen, 
            m_pStartShape, 
            m_eStartShapeType);
    }

    HRESULT SetCustomEndShape(
        __in_ecount(1) const CShape &shape,
            // The line shape's geometry        
        IN FLOAT        rInset,
            // The amount the path will be trimmed
        IN FLOAT        rAnchorDistance,
            // The -Y coordinate of the anchor point 
        IN bool         fFill,
            // Fill the shape if true 
        IN bool         fStroke,
            // Stroke the shape if true
        __in_ecount_opt(1) CPlainPen    *pPen)
            // An overriding pen for strokes  (NULL OK)
    {
        return SetCustomLineShape(
            shape, 
            rInset, 
            rAnchorDistance, 
            fFill, 
            fStroke,
            pPen, 
            m_pEndShape, 
            m_eEndShapeType);
    }

    REAL GetWidth() const
    {
        return m_oGeom.GetWidth();
    }
            
    void SetWidth(
        REAL val)
    {
        m_oGeom.SetWidth(val);
    }

    REAL GetHeight() const
    {
        return m_oGeom.GetHeight();
    }
            
    void SetHeight(
        REAL val)
    {
        m_oGeom.SetHeight(val);
    }

    REAL GetAngle() const
    {
        return m_oGeom.GetAngle();
    }

    void SetAngle(
        REAL val)
    {
        m_oGeom.SetAngle(val);
    }

    const __out_ecount(1) CLineShape *GetStartShape()  const
    {
        return m_pStartShape;
    }
    
    const __out_ecount(1) CLineShape *GetEndShape() const
    {
        return m_pEndShape;
    }

    MilPenCap::Enum GetStartCap() const
    {
        return m_oGeom.GetStartCap();
    }
    
    void SetStartCap(
        MilPenCap::Enum val)
    {
        m_oGeom.SetStartCap(val);
    }

    MilPenCap::Enum GetEndCap() const
    {
        return m_oGeom.GetEndCap();
    }
    
    void SetEndCap(
        MilPenCap::Enum val)
    {
        m_oGeom.SetEndCap(val);
    }

    MilPenCap::Enum GetDashCap() const
    {
        return m_oGeom.GetDashCap();
    }

    void SetDashCap(
        MilPenCap::Enum val)
    {
        m_oGeom.SetDashCap(val);
    }

    MilLineJoin::Enum GetJoin() const
    {
        return m_oGeom.GetJoin();
    }
    
    void SetJoin(
        MilLineJoin::Enum val)
    {
        m_oGeom.SetJoin(val);
    }

    REAL GetMiterLimit() const
    {
        return m_oGeom.GetMiterLimit();
    }
    
    void SetMiterLimit(
        REAL val)
    {
        m_oGeom.SetMiterLimit(val);
    }

    MilDashStyle::Enum GetDashStyle() const
    {
        return m_eDashStyle;
    }
    
    REAL GetDashOffset() const
    {
        return m_rDashOffset;
    }
    
    void SetDashOffset(
        REAL val)
    {
        m_rDashOffset = val;
    }

    INT GetDashCount() const
    {
        return m_rgDashes.GetCount(); 
    }
    
    REAL GetDash(INT i) const
    {
        return m_rgDashes[i];
    }

    __outro_ecount(1) const CPenGeometry &GetGeometry() const
    {
        return m_oGeom;
    }

    REAL Get90DegreeBevelOffset() const
    {
        REAL rMiterLimit = GetMiterLimit();
        REAL rWidth = GetWidth();

        MilLineJoin::Enum join = GetJoin();

        Assert(join != MilLineJoin::Round);
        Assert(IsCircular());

        REAL rBevelOffset = 0.0f;
       
        switch (join)
        {
            case MilLineJoin::Miter:
                rBevelOffset = ClampReal(
                    2.0f - static_cast<REAL>(SQRT_2) * rMiterLimit, 
                    0.0f /* minimum */,
                    1.0f /* maximum */); 
                break;
            case MilLineJoin::MiterClipped:
                if (rMiterLimit > 0.5f * static_cast<REAL>(SQRT_2))
                {
                    rBevelOffset = 1.0f;
                }
                else
                {
                    rBevelOffset = 0.0f;
                }
                break;
            case MilLineJoin::Bevel:
                rBevelOffset = 1.0f;
                break;
            default:
                RIP("Unexpected line join type");
        }

        return rBevelOffset * 0.5f * rWidth;
    }
    
    HRESULT GetExtents(__out_ecount(1) REAL &rExtents) const;
            
    bool CanFillBoundsExceedStrokeBounds(__in_ecount(1) const IShapeData &refData) const
    {
        // For now the filled geometry may exceed the stroke only if the pen
        // is empty, dashed or compound, or if the geometry has gaps.
        // This may change with additional features.
        return IsEmpty() || IsCompound() || MilDashStyle::Solid != m_eDashStyle || refData.HasGaps();
    }

    // Private methods
protected:
    HRESULT SetCannedLineShape(
        IN MilLineShape::Enum   eType,
            // The canned shape type
        __deref_out_ecount(1) CLineShape    *&pDest,
            // The destination for the line shape
        __out_ecount(1) MilLineShape::Enum  &eTypeToSet,
            // The destination type to set
        __out_ecount(1) MilPenCap::Enum   &eCap);
            // Line cap there, set    
    
    HRESULT SetCustomLineShape(
        __in_ecount(1) const CShape &shape,
            // The line shape's geometry        
        IN FLOAT        rInset,
            // The amount the path will be trimmed
        IN FLOAT        rAnchorDistance,
            // The -Y coordinate of the anchor point 
        IN bool         fFill,
            // Fill the shape if true 
        IN bool         fStroke,
            // Stroke the shape if true
        __in_ecount_opt(1) CPlainPen    *pPen,
            // An overriding pen for strokes
        __deref_out_ecount(1) CLineShape  *&pDest,
            // The destination shape's (m_pStartShape or m_pEndShape)
        __out_ecount(1) MilLineShape::Enum &eType);
            // The type (m_eStartShapeType of m_pEndShapeType) to set to Custom
    
    // Data
protected:
    CPenGeometry    m_oGeom;    

    MilDashStyle::Enum   m_eDashStyle;
    REAL           m_rDashOffset;
    
    DynArray<REAL>  m_rgDashes;
#ifdef COMPOUND_PEN_IMPLEMENTED    
    DynArray<REAL>  m_rgCompound;
#endif // COMPOUND_PEN_IMPLEMENTED
    MilLineShape::Enum   m_eStartShapeType;
    MilLineShape::Enum   m_eEndShapeType;
    CLineShape     *m_pStartShape;
    CLineShape     *m_pEndShape;
};

#endif

